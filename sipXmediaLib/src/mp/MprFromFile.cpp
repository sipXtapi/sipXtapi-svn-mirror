//  
// Copyright (C) 2005-2007 SIPez LLC. 
// Licensed to SIPfoundry under a Contributor Agreement. 
//
// Copyright (C) 2004-2007 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////


// SYSTEM INCLUDES
#include <assert.h>
#include <os/fstream>
#include <stdio.h>
#include <math.h>

#ifdef __pingtel_on_posix__
#include <stdlib.h>
#endif

// APPLICATION INCLUDES
#include "os/OsDefs.h"
#include "os/OsEvent.h"
#include "mp/MpTypes.h"
#include "mp/MpBuf.h"
#include "mp/MprFromFile.h"
#include "mp/MpAudioAbstract.h"
#include "mp/MpAudioFileOpen.h"
#include "mp/MpAudioUtils.h"
#include "mp/MpAudioWaveFileRead.h"
#include "mp/MpFromFileStartResourceMsg.h"
#include "mp/mpau.h"
#include "mp/MpMisc.h"
#include "mp/MpFlowGraphBase.h"
#include "os/OsSysLog.h"
#include "os/OsProtectEventMgr.h"
#include "os/OsDateTime.h"
#include "os/OsTime.h"
#include "mp/MpResNotificationMsg.h"
#include "mp/MprnProgressMsg.h"
#include "mp/MpProgressResourceMsg.h"
#include "mp/MpResampler.h"


// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES

// CONSTANTS
const unsigned int MprFromFile::sFromFileReadBufferSize = 8000;

static const unsigned int MAXFILESIZE = 50000000;
static const unsigned int MINFILESIZE = 8000;
extern int      samplesPerSecond;
extern int      bitsPerSample;
extern int      samplesPerFrame;

// STATIC VARIABLE INITIALIZATIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
MprFromFile::MprFromFile(const UtlString& rName)
: MpAudioResource(rName, 0, 1, 1, 1)
, mpFileBuffer(NULL)
, mFileRepeat(FALSE)
, mpNotify(NULL)
, mPaused(FALSE)
, mProgressIntervalMS(0)
{
}

// Destructor
MprFromFile::~MprFromFile()
{
   if(mpFileBuffer) delete mpFileBuffer;
   mpFileBuffer = NULL;
}

/* ============================ MANIPULATORS ============================== */

OsStatus MprFromFile::playBuffer(const char* audioBuffer, unsigned long bufSize, 
                                 int type, UtlBoolean repeat, OsProtectedEvent* notify)
{
   UtlString* fgAudBuffer = NULL;
   OsStatus res = genericAudioBufToFGAudioBuf(fgAudBuffer, audioBuffer, 
                                              bufSize, type);

   if(res == OS_SUCCESS)
   {
      // Tell CpCall that we've copied the data out of the buffer, so it
      // can continue processing.
      if (notify && OS_ALREADY_SIGNALED == notify->signal(0))
      {
         OsProtectEventMgr* eventMgr = OsProtectEventMgr::getEventMgr();
         eventMgr->release(notify);
      }

      // Don't pass the event in the PLAY_FILE message.
      // That means that the file-play process can't pass signals
      // back.  But we have already released the OsProtectedEvent.
      MpFlowGraphMsg msg(PLAY_FILE, this, NULL, fgAudBuffer,
                         repeat ? PLAY_REPEAT : PLAY_ONCE, 0);
      res = postMessage(msg);
   }

   return res;
}


OsStatus MprFromFile::playBuffer(const UtlString& namedResource, 
                                 OsMsgQ& fgQ, 
                                 const char* audioBuffer, 
                                 unsigned long bufSize, int type, 
                                 UtlBoolean repeat, 
                                 OsNotification* evt)
{
   UtlString* fgAudBuffer = NULL;
   OsStatus stat = genericAudioBufToFGAudioBuf(fgAudBuffer, audioBuffer,
                                               bufSize, type);

   if(stat == OS_SUCCESS)
   {
      MpFromFileStartResourceMsg msg(namedResource, fgAudBuffer, repeat, evt);
      stat = fgQ.send(msg, sOperationQueueTimeout);
   }
   return stat;
}



// old play file w/ file name & repeat option
OsStatus MprFromFile::playFile(const char* audioFileName, 
                               UtlBoolean repeat,
                               OsNotification* notify)
{
   OsStatus stat;
   UtlString* audioBuffer = NULL;
   assert(getFlowGraph() != NULL);
   stat = readAudioFile(getFlowGraph()->getSamplesPerSec(), 
                        audioBuffer, audioFileName, notify);

   //create a msg from the buffer
   if (audioBuffer && audioBuffer->length())
   {
      MpFlowGraphMsg msg(PLAY_FILE, this, notify, audioBuffer,
                         repeat ? PLAY_REPEAT : PLAY_ONCE, 0);

      //now post the msg (with the audio data) to be played
      stat = postMessage(msg);
   }

   return stat;
}

OsStatus MprFromFile::playFile(const UtlString& namedResource, 
                               OsMsgQ& fgQ, 
                               uint32_t fgSampleRate,
                               const UtlString& filename, 
                               const UtlBoolean& repeat,
                               OsNotification* notify)
{
   UtlString* audioBuffer = NULL;
   OsStatus stat = readAudioFile(fgSampleRate, audioBuffer, filename, notify);
   if(stat == OS_SUCCESS)
   {
      MpFromFileStartResourceMsg msg(namedResource, audioBuffer, repeat, notify);
      stat = fgQ.send(msg, sOperationQueueTimeout);
   }
   return stat;
}

// stop file play
OsStatus MprFromFile::stopFile(void)
{
   MpFlowGraphMsg msg(STOP_FILE, this, NULL, NULL, 0, 0);
   return postMessage(msg);
}

OsStatus MprFromFile::stopFile(const UtlString& namedResource, 
                               OsMsgQ& fgQ)
{
   MpResourceMsg msg(MpResourceMsg::MPRM_FROMFILE_STOP, namedResource);
   return fgQ.send(msg, sOperationQueueTimeout);
}

OsStatus MprFromFile::pauseFile(const UtlString& namedResource, 
                                OsMsgQ& fgQ)
{
   MpResourceMsg msg(MpResourceMsg::MPRM_FROMFILE_PAUSE, namedResource);
   return fgQ.send(msg, sOperationQueueTimeout);
}

OsStatus MprFromFile::resumeFile(const UtlString& namedResource,
                                 OsMsgQ& fgQ)
{
   MpResourceMsg msg(MpResourceMsg::MPRM_FROMFILE_RESUME, namedResource);
   return fgQ.send(msg, sOperationQueueTimeout);
}

OsStatus MprFromFile::sendProgressPeriod(const UtlString& namedResource, 
                                         OsMsgQ& fgQ, 
                                         int32_t updatePeriodMS)
{
   MpProgressResourceMsg msg(MpResourceMsg::MPRM_FROMFILE_SEND_PROGRESS,
                             namedResource, updatePeriodMS);
   return fgQ.send(msg, sOperationQueueTimeout);
}

UtlBoolean MprFromFile::enable(void) //$$$
{
   if (mpNotify) {
      mpNotify->signal(PLAYING);
   }
   return MpResource::enable();
}

UtlBoolean MprFromFile::disable(void) //$$$
{
   if (mpNotify) {
      mpNotify->signal(PLAY_STOPPED);
      mpNotify->signal(PLAY_FINISHED);
      mpNotify = NULL;
   }
   return MpResource::disable();
}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

OsStatus MprFromFile::genericAudioBufToFGAudioBuf(UtlString*& fgAudioBuf, 
                                                  const char* audioBuffer, 
                                                  unsigned long bufSize, 
                                                  int type)
{
   OsStatus stat = OS_SUCCESS;
   char* convertedBuffer = NULL;

   assert(fgAudioBuf == NULL); // assume UtlString buffer pointer is null.
   fgAudioBuf = new UtlString();

   if (fgAudioBuf)
   {
      switch(type)
      {
      case 0 : fgAudioBuf->append(audioBuffer,bufSize);
         break;

      case 1 : convertedBuffer = new char[bufSize*2];
         //NOTE by Keith Kyzivat - this code was pulled directly from
         // the old implementation of playBuffer... obviously broken here.
         //TODO: actually convert the buffer
         fgAudioBuf->append(convertedBuffer,bufSize);
         delete[] convertedBuffer; 
         break;
      }
   }
   else
   {
      stat = OS_INVALID_ARGUMENT;
   }

   return stat;
}

OsStatus MprFromFile::readAudioFile(uint32_t fgSampleRate,
                                    UtlString*& audioBuffer,
                                    const char* audioFileName,
                                    OsNotification* notify)
{
   char* charBuffer = NULL;
   FILE* audioFilePtr = NULL;
   int iTotalChannels = 1;
   uint32_t filesize;
   uint32_t trueFilesize;
   int samplesReaded;
   int compressionType = 0;
   int channelsMin = 1, channelsMax = 2, channelsPreferred = 0;
   long rateMin = 8000, rateMax = 44100, ratePreferred = 22050;
   UtlBoolean bDetectedFormatIsOk = TRUE;
   MpAudioAbstract *audioFile = NULL;

   // Assume audioBuffer passed in is NULL..
   assert(audioBuffer == NULL);
   audioBuffer = NULL;


   if (!audioFileName)
      return OS_INVALID_ARGUMENT;

   ifstream inputFile(audioFileName,ios::in|ios::binary);

   if (!inputFile.good())
   {
      return OS_INVALID_ARGUMENT;
   }

   //get file size
   inputFile.seekg(0, ios::end);
   filesize = trueFilesize = inputFile.tellg();
   inputFile.seekg(0);

   //we have to have at least one sample to play
   if (trueFilesize < sizeof(AudioSample))  
   {
      osPrintf("WARNING: %s contains less than one sample to play. "
         "Skipping play.\n", audioFileName);
      return OS_INVALID_ARGUMENT;
   }

   if (trueFilesize > MAXFILESIZE)
   {
      osPrintf("playFile('%s') WARNING:\n"
         "    length (%lu) exceeds size limit (%d)\n",
         audioFileName, trueFilesize, MAXFILESIZE);
      filesize = MAXFILESIZE;
   }

   if (trueFilesize < MINFILESIZE)
   {
      osPrintf("playFile('%s') WARNING:\n"
         "    length (%lu) is suspiciously short!\n",
         audioFileName, trueFilesize);
   }


   audioFile = MpOpenFormat(inputFile);
   //if we have an audioFile object, then it must be a known file type
   //otherwise, lets treat it as RAW
   if (audioFile)
   {
      if (audioFile->isOk())
      {
         audioFile->minMaxChannels(&channelsMin,
            &channelsMax, &channelsPreferred);

         if (channelsMin > channelsMax) 
         {
            osPrintf("Couldn't negotiate channels.\n");
            bDetectedFormatIsOk = FALSE;
         }

         audioFile->minMaxSamplingRate(&rateMin,&rateMax,&ratePreferred);
         if (rateMin > rateMax) 
         {
            osPrintf("Couldn't negotiate rate.\n");
            bDetectedFormatIsOk = FALSE;
         }
      }
      else
         bDetectedFormatIsOk = FALSE;

      if (bDetectedFormatIsOk)
      {
         iTotalChannels = channelsPreferred;
         compressionType = audioFile->getDecompressionType();
      }
      else
      {
         osPrintf("\nERROR: Could not detect format correctly. "
            "Should be AU or WAV or RAW\n");
      }

      // First, figure out which kind of file it is
      if (bDetectedFormatIsOk && 
         audioFile->getAudioFormat() == AUDIO_FORMAT_WAV)
      {

         // Get actual data size without header.
         filesize = audioFile->getBytesSize();

         switch(compressionType) 
         {
         case MpAudioWaveFileRead::DePcm8Unsigned: //8
            // We'll convert it to 16 bit
            filesize *= sizeof(AudioSample);
            charBuffer = (char*)malloc(filesize);
            samplesReaded = audioFile->getSamples((AudioSample*)charBuffer,
                                                  filesize);

            if (samplesReaded) 
            {
               assert(samplesReaded*sizeof(AudioSample) == filesize);

               // Convert to mono if needed
               if (channelsPreferred > 1)
                  filesize = mergeChannels(charBuffer, filesize, iTotalChannels);

               // charBuffer will point to a new buffer holding the resampled
               // data and filesize will be updated with the new buffer size
               // after this call.
               if(allocateAndResample(charBuffer, filesize, ratePreferred, 
                                      fgSampleRate) == FALSE)
               {
                  if(notify) notify->signal(INVALID_SETUP);
                  break;
               }
            }
            else
            {
               if (notify) notify->signal(INVALID_SETUP);
            }
            break;

         case MpAudioWaveFileRead::DePcm16LsbSigned: // 16
            charBuffer = (char*)malloc(filesize);
            samplesReaded = audioFile->getSamples((AudioSample*)charBuffer,
                                                  filesize/sizeof(AudioSample));
            if (samplesReaded)
            {
               assert(samplesReaded*sizeof(AudioSample) == filesize);

               // Convert to mono if needed
               if (iTotalChannels > 1)
                  filesize = mergeChannels(charBuffer, filesize, iTotalChannels);

               // charBuffer will point to a new buffer holding the resampled
               // data and filesize will be updated with the new buffer size
               // after this call.
               if(allocateAndResample(charBuffer, filesize, ratePreferred,
                                      fgSampleRate) == FALSE)
               {
                  if(notify) notify->signal(INVALID_SETUP);
                  break;
               }
            }
            else
            {
               if (notify) notify->signal(INVALID_SETUP);
            }
            break;
         }
      }
      else
         if (bDetectedFormatIsOk && 
            audioFile->getAudioFormat() == AUDIO_FORMAT_AU)
         {

            // Get actual data size without header.
            filesize = audioFile->getBytesSize();

            switch(compressionType)
            {
            case MpAuRead::DePcm8Unsigned:
               break; //do nothing for this format

            case MpAuRead::DeG711MuLaw:
               charBuffer = (char*)malloc(filesize*2);
               samplesReaded = audioFile->getSamples((AudioSample*)charBuffer, filesize);
               if (samplesReaded) 
               {

                  //it's now 16 bit so it's twice as long
                  filesize *= sizeof(AudioSample);

                  // Convert to mono if needed
                  if (channelsPreferred > 1)
                     filesize = mergeChannels(charBuffer, filesize, iTotalChannels);

                  // charBuffer will point to a new buffer holding the resampled
                  // data and filesize will be updated with the new buffer size
                  // after this call.
                  if(allocateAndResample(charBuffer, filesize, ratePreferred,
                                         fgSampleRate) == FALSE)
                  {
                     if(notify) notify->signal(INVALID_SETUP);
                     break;
                  }
               }
               else
               {
                  if (notify) notify->signal(INVALID_SETUP);
               }
               break;

            case MpAuRead::DePcm16MsbSigned:
               charBuffer = (char*)malloc(filesize);
               samplesReaded = audioFile->getSamples((AudioSample*)charBuffer,
                                                     filesize/sizeof(AudioSample));
               if (samplesReaded) 
               {
                  assert(samplesReaded*sizeof(AudioSample) == filesize);

                  // Convert to mono if needed
                  if (channelsPreferred > 1)
                     filesize = mergeChannels(charBuffer, filesize, iTotalChannels);

                  // charBuffer will point to a new buffer holding the resampled
                  // data and filesize will be updated with the new buffer size
                  // after this call.
                  if(allocateAndResample(charBuffer, filesize, ratePreferred,
                                         fgSampleRate) == FALSE)
                  {
                     if(notify) notify->signal(INVALID_SETUP);
                     break;
                  }
               }
               else
               {
                  if (notify) notify->signal(INVALID_SETUP);
               }
               break;
            }
         } 
         else 
         {
            OsSysLog::add(FAC_MP, PRI_ERR, 
               "ERROR: Detected audio file is bad.  "
               "Must be MONO, 16bit signed wav or u-law au");
         }

         //remove object used to determine rate, compression, etc.
         delete audioFile;
         audioFile = NULL;
   }
   else
   {
#if 0
      osPrintf("AudioFile: raw file\n");
#endif

      // if we cannot determine the format of the audio file,
      // and if the ext of the file is .ulaw, we assume it is a no-header
      // raw file of ulaw audio, 8 bit, 8kHZ.
      if (strstr(audioFileName, ".ulaw"))
      {
         ratePreferred = 8000;
         channelsPreferred = 1;
         audioFile = new MpAuRead(inputFile, 1);
         if (audioFile)
         {
            filesize *= sizeof(AudioSample);
            charBuffer = (char*)malloc(filesize);

            samplesReaded = audioFile->getSamples((AudioSample*)charBuffer,
                                                  filesize/sizeof(AudioSample));
            if (!samplesReaded) 
            {
               if (notify) notify->signal(INVALID_SETUP);
            }
         }
      }
      else // the file extension is not .ulaw ... 
      {
         if (0 != (audioFilePtr = fopen(audioFileName, "rb")))
         {
            unsigned int cbIdx = 0;
            int bytesRead = 0;
            charBuffer = (char*)malloc(filesize);
            assert(charBuffer != NULL); // Assume malloc succeeds.

            // Read in the unknown audio file a chunk at a time.
            // (specified by sFromFileReadBufferSize)
            while((cbIdx < filesize) &&
               ((bytesRead = fread(charBuffer+cbIdx, 1, 
               sFromFileReadBufferSize, 
               audioFilePtr)) > 0))
            {
               cbIdx += bytesRead;
            }

            // Now that we're done with the unknown raw audio file
            // close it up.
            fclose(audioFilePtr);
         }
      }
   }

   // Now we copy over the char buffer data to UtlString for use in
   // messages.
   if(charBuffer != NULL)
   {
      audioBuffer = new UtlString();
      if (audioBuffer)
      {
         audioBuffer->append(charBuffer, filesize);
#if 0
         osPrintf("Audio Buffer length: %d\n", audioBuffer->length());
#endif
      }
      free(charBuffer);
   }

   return OS_SUCCESS;
}

UtlBoolean MprFromFile::allocateAndResample(char*& audBuf,
                                            uint32_t& audBufSz,
                                            uint32_t inRate,
                                            uint32_t outRate)
{
   // Check if the rates match -- if so, no need to resample - it's already done!
   if(inRate == outRate)
   {
      return TRUE;
   }

   // Malloc up a new chunk of memory for resampling to.
   uint32_t outBufSize = audBufSz * (uint32_t)ceil(outRate/(float)inRate);
   MpAudioSample* pOutSamples = (MpAudioSample*)malloc(outBufSize);
   if(pOutSamples == NULL)
   {
      OsSysLog::add(FAC_MP, PRI_ERR, 
                    "ERROR: Failed to allocate a buffer to resample to.");
      return FALSE;
   }

   uint32_t inSamplesProcessed = 0;
   uint32_t outSamplesWritten = 0;
   OsStatus resampleStat = OS_SUCCESS;
   MpResampler resampler(1, inRate, outRate);
   resampleStat = resampler.resample(0, (MpAudioSample*)audBuf, audBufSz/sizeof(MpAudioSample), 
                                     inSamplesProcessed, 
                                     pOutSamples, outBufSize/sizeof(MpAudioSample), 
                                     outSamplesWritten);
   if(resampleStat != OS_SUCCESS)
   {
      OsSysLog::add(FAC_MP, PRI_ERR,
                    "ERROR: Resampler failed with status: %d", 
                    resampleStat);
      return FALSE;
   }

   // Ok, now free charBuffer and point the new resampled buffer
   // to it.
   free(audBuf);
   audBuf = (char*)pOutSamples;
   audBufSz = outBufSize;

   return TRUE;
}

// This one is private -- only used internally.
OsStatus MprFromFile::finishFile()
{
   OsMsgQ* fgQ = getFlowGraph()->getMsgQ();
   assert(fgQ != NULL);

   MpResourceMsg msg((MpResourceMsg::MpResourceMsgType)MPRM_FROMFILE_FINISH, getName());
   return fgQ->send(msg, sOperationQueueTimeout);
}

UtlBoolean MprFromFile::doProcessFrame(MpBufPtr inBufs[],
                                       MpBufPtr outBufs[],
                                       int inBufsSize,
                                       int outBufsSize,
                                       UtlBoolean isEnabled,
                                       int samplesPerFrame,
                                       int samplesPerSecond)
{
   MpAudioBufPtr out;
   MpAudioSample *outbuf;
   int count;
   int bytesLeft;

   // There's nothing to do if the output buffers or the number
   // of samples per frame are zero, so just return.
   if (outBufsSize == 0 || samplesPerFrame == 0)
   {
       return FALSE;
   }

   // If we're enabled and not paused, then do playback,
   // otherwise passthrough.
   if (isEnabled && !mPaused) 
   {
      if (mpFileBuffer)
      {
         // Get new buffer
         out = MpMisc.RawAudioPool->getBuffer();
         if (!out.isValid())
         {
            return FALSE;
         }
         out->setSpeechType(MpAudioBuf::MP_SPEECH_TONE);
         out->setSamplesNumber(samplesPerFrame);
         count = out->getSamplesNumber();
         outbuf = out->getSamplesWritePtr();

         int bytesPerFrame = count * sizeof(MpAudioSample);
         int bufferLength = mpFileBuffer->length();
         int totalBytesRead = 0;

         if(mFileBufferIndex < bufferLength)
         {
            totalBytesRead = bufferLength - mFileBufferIndex;
            totalBytesRead = sipx_min(totalBytesRead, bytesPerFrame);
            memcpy(outbuf, &(mpFileBuffer->data()[mFileBufferIndex]),
                   totalBytesRead);
            mFileBufferIndex += totalBytesRead;
         }

         if (mFileRepeat) 
         {
            bytesLeft = 1;
            while((totalBytesRead < bytesPerFrame) && (bytesLeft > 0))
            {
               mFileBufferIndex = 0;
               bytesLeft = sipx_min(bufferLength - mFileBufferIndex,
                               bytesPerFrame - totalBytesRead);
               memcpy(&outbuf[(totalBytesRead/sizeof(MpAudioSample))],
                      &(mpFileBuffer->data()[mFileBufferIndex]), bytesLeft);
               totalBytesRead += bytesLeft;
               mFileBufferIndex += bytesLeft;
            }
         } else 
         {
            if (mFileBufferIndex >= bufferLength) 
            {
               // We're done playing..
               // zero out the remaining bytes in the frame after the end
               // of the real data before sending it off - it could be garbage.
               bytesLeft = bytesPerFrame - totalBytesRead;
               memset(&outbuf[(totalBytesRead/sizeof(MpAudioSample))], 0, bytesLeft);

               // Send a message to tell this resource to stop playing the file
               // this resets some state, and sends a notification.
               finishFile();
            }
         }

         // Check to see if we need to make a progress update.
         OsTime curTime;
         OsDateTime::getCurTime(curTime);
         // Info about clock rollover here -- subtraction of OsTimes -- this will
         // not have issue with rollover -- issues with rollover would happen in 2038 
         // (2^31 seconds)
         // Comparison rollover -- cvtToMsecs() is a long - 2^31 msecs - 24.85 days
         // So, given that specification of interval is with int32_t, that means
         // interval will never be > than this rollover, so no need to worry,
         // and we can assume doProcessFrame will be executed in a timely fashion.
         if(mProgressIntervalMS > 0 &&
            (mLastProgressUpdate - curTime).cvtToMsecs() >= mProgressIntervalMS)
         {
            // We get here if there *is* a progress interval, and if the # ms
            // passed since the last progress update is >= the progress interval.
            // In this case, we need to send out a progress notification message.
            unsigned amountPlayedMS = 
               mFileBufferIndex / sizeof(MpAudioSample) / samplesPerSecond;
            unsigned totalBufferMS = 
               mpFileBuffer->length() / sizeof(MpAudioSample) / samplesPerSecond;

            MprnProgressMsg progressMsg(MpResNotificationMsg::MPRNM_FROMFILE_PROGRESS,
                                        getName(), amountPlayedMS, totalBufferMS);
            sendNotification(progressMsg);
         }
      }
   }
   else
   {
      // Resource is disabled. Passthrough input data
      out.swap(inBufs[0]);
   }

   // Push audio data downstream
   outBufs[0] = out;

   return TRUE;
}

// Handle messages for this resource.

// This is used in both old and new messaging schemes to initialize everything
// and start playing a buffer, when a play is requested.
UtlBoolean MprFromFile::handlePlay(OsNotification* pNotifier, 
                                   UtlString* pBuffer, UtlBoolean repeat)
{
   // Enable this resource - as it's disabled automatically when the last file ends.
   enable();

   if(mpFileBuffer) delete mpFileBuffer;
   if (mpNotify) 
   {
      mpNotify->signal(PLAY_FINISHED);
   }
   mpNotify = pNotifier;
   mpFileBuffer = pBuffer;
   if(mpFileBuffer) 
   {
      mFileBufferIndex = 0;
      mFileRepeat = repeat;
   }

   // Notify, indicating we're started, if notfs enabled.
   sendNotification(MpResNotificationMsg::MPRNM_FROMFILE_STARTED);

   return TRUE;
}

// this is used in both old and new messaging schemes to do reset state
// and send notification when stop is requested.
UtlBoolean MprFromFile::handleStop(UtlBoolean finished)
{
   MpResNotificationMsg::RNMsgType msgType = (finished == TRUE) ?
      MpResNotificationMsg::MPRNM_FROMFILE_FINISHED :
      MpResNotificationMsg::MPRNM_FROMFILE_STOPPED;

   // Send a notification -- we don't really care at this level if
   // it succeeded or not.
   sendNotification(msgType);

   // Cleanup.
   delete mpFileBuffer;
   mpFileBuffer = NULL;
   mFileBufferIndex = 0;
   mPaused = FALSE;
   disable();
   return TRUE;
}

UtlBoolean MprFromFile::handlePause()
{
   UtlBoolean retVal = FALSE;
   if(isEnabled() && mpFileBuffer != NULL)
   {
      mPaused = TRUE;
      sendNotification(MpResNotificationMsg::MPRNM_FROMFILE_PAUSED);
      retVal = TRUE;
   }
   return retVal;
}

UtlBoolean MprFromFile::handleResume()
{
   UtlBoolean retVal = FALSE;
   if(isEnabled()
      && mpFileBuffer != NULL
      && mPaused == TRUE)
   {
      mPaused = FALSE;
      sendNotification(MpResNotificationMsg::MPRNM_FROMFILE_RESUMED);
      retVal = TRUE;
   }
   return retVal;
}

UtlBoolean MprFromFile::handleSetUpdatePeriod(int32_t periodMS)
{
   // Set the 'last progress update' to now, since there wasn't one before.
   OsDateTime::getCurTime(mLastProgressUpdate);
   // and set the period to the new one provided.
   mProgressIntervalMS = periodMS;
   return TRUE;
}

// Old flowgraph message approach to sending messages
// DEPRECATED.  This will be removed once new messaging infrastructure
// is solid.
UtlBoolean MprFromFile::handleMessage(MpFlowGraphMsg& rMsg)
{
   switch (rMsg.getMsg()) 
   {
   case PLAY_FILE:
      return handlePlay((OsNotification*)rMsg.getPtr1(),
                        (UtlString*)rMsg.getPtr2(),
                        (rMsg.getInt1() == PLAY_ONCE) ? FALSE : TRUE);
      break;

   case STOP_FILE:
      return handleStop();
      break;

   default:
      return MpAudioResource::handleMessage(rMsg);
      break;
   }
   return TRUE;
}


// New resource message handling.  This is part of the new
// messaging infrastructure (2007).
UtlBoolean MprFromFile::handleMessage(MpResourceMsg& rMsg)
{
   UtlBoolean msgHandled = FALSE;

   MpFromFileStartResourceMsg* ffsRMsg = NULL;
   switch (rMsg.getMsg()) 
   {
   case MpResourceMsg::MPRM_FROMFILE_START:
      ffsRMsg = (MpFromFileStartResourceMsg*)&rMsg;
      msgHandled = handlePlay(ffsRMsg->getOsNotification(), 
                              ffsRMsg->getAudioBuffer(),
                              ffsRMsg->isRepeating());
      break;

   case MpResourceMsg::MPRM_FROMFILE_STOP:
      msgHandled = handleStop();
      break;

   case MpResourceMsg::MPRM_FROMFILE_PAUSE:
      msgHandled = handlePause();
      break;

   case MpResourceMsg::MPRM_FROMFILE_RESUME:
      msgHandled = handleResume();
      break;

   case MpResourceMsg::MPRM_FROMFILE_SEND_PROGRESS:
      msgHandled = handleSetUpdatePeriod(((MpProgressResourceMsg*)&rMsg)->getUpdatePeriodMS());
      break;      

   case MPRM_FROMFILE_FINISH:
      // Stop, but indicate finished.
      msgHandled = handleStop(TRUE);
      break;

   default:
      // If we don't handle the message here, let our parent try.
      msgHandled = MpResource::handleMessage(rMsg); 
      break;
   }
   return msgHandled;
}

/* ============================ FUNCTIONS ================================= */


