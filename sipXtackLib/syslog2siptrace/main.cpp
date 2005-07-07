// 
// 
// Copyright (C) 2004 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
// 
// Copyright (C) 2004 Pingtel Corp.
// Licensed to SIPfoundry under a Contributor Agreement.
// 
// $$
//////////////////////////////////////////////////////////////////////////////

// Cloned from syslogviewer

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#if defined(_WIN32)
#   include <io.h>
#   include <string.h>
#elif defined(__linux__)
#   include <unistd.h>
#endif

#define BUFFER_SIZE 8192

#include <os/OsDefs.h>
#include <os/OsSysLog.h>
#include <net/NameValueTokenizer.h>
#include <net/SipMessage.h>

void writeMessageNodesBegin(int outputFileDescriptor)
{
    UtlString nodeBegin("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n<sipTrace>\n");
    write(outputFileDescriptor, nodeBegin.data(), nodeBegin.length());
}

void writeMessageNodesEnd(int outputFileDescriptor)
{
    UtlString nodeEnd("</sipTrace>\n");
    write(outputFileDescriptor, nodeEnd.data(), nodeEnd.length());

}

void writeBranchNodeBegin(int outputFileDescriptor)
{
    UtlString nodeBegin("\t<branchNode>\n");
    write(outputFileDescriptor, nodeBegin.data(), nodeBegin.length());

}

void writeBranchNodeEnd(int outputFileDescriptor)
{
    UtlString nodeEnd("\t</branchNode>\n");
    write(outputFileDescriptor, nodeEnd.data(), nodeEnd.length());

}

void writeBranchSetBegin(int outputFileDescriptor)
{
    UtlString nodeBegin("\t\t<branchIdSet>\n");
    write(outputFileDescriptor, nodeBegin.data(), nodeBegin.length());

}

void writeBranchSetEnd(int outputFileDescriptor)
{
    UtlString nodeEnd("\t\t</branchIdSet>\n");
    write(outputFileDescriptor, nodeEnd.data(), nodeEnd.length());

}

void writeBranchId(int outputFileDescriptor,
                   UtlString& branchId)
{
    NameValueTokenizer::frontBackTrim(&branchId, " \t\n\r");
    UtlString node("\t\t\t<branchId>");
    node.append(branchId);
    node.append("</branchId>\n");

    write(outputFileDescriptor, node.data(), node.length());
}

void writeBranchNodeData(int outputFileDescriptor, 
                      UtlString& time,
                      UtlString& source,
                      UtlString& destination,
                      UtlString& sourceAddress,
                      UtlString& destinationAddress,
                      UtlString& transactionId,
                      UtlString& frameId,
                      UtlString& method,
                      UtlString& responseCode,
                      UtlString& responseText,
                      UtlString& message)
{
    NameValueTokenizer::frontBackTrim(&time, " \t\n\r");
    NameValueTokenizer::frontBackTrim(&source, " \t\n\r");
    NameValueTokenizer::frontBackTrim(&destination, " \t\n\r");
    NameValueTokenizer::frontBackTrim(&sourceAddress, " \t\n\r");
    NameValueTokenizer::frontBackTrim(&destinationAddress, " \t\n\r");
    NameValueTokenizer::frontBackTrim(&transactionId, " \t\n\r");
    NameValueTokenizer::frontBackTrim(&frameId, " \t\n\r");
    NameValueTokenizer::frontBackTrim(&method, " \t\n\r");
    NameValueTokenizer::frontBackTrim(&responseCode, " \t\n\r");
    NameValueTokenizer::frontBackTrim(&responseText, " \t\n\r");
    //NameValueTokenizer::frontBackTrim(&message, " \t\n\r");

    UtlString node("\t\t<time>");
    node.append(time);
    node.append("</time>\n");

    if(!source.isNull())
    {
        node.append("\t\t<source>");
        node.append(source);
        node.append("</source>\n");
    }

    if(!destination.isNull())
    {
        node.append("\t\t<destination>");
        node.append(destination);
        node.append("</destination>\n");
    }

    node.append("\t\t<sourceAddress>");
    node.append(sourceAddress);
    node.append("</sourceAddress>\n");

    node.append("\t\t<destinationAddress>");
    node.append(destinationAddress);
    node.append("</destinationAddress>\n");

    node.append("\t\t<transactionId>");
    node.append(transactionId);
    node.append("</transactionId>\n");

    if(!method.isNull())
    {
        node.append("\t\t<method>");
        node.append(method);
        node.append("</method>\n");
    }
    else
    {
        node.append("\t\t<responseCode>");
        node.append(responseCode);
        node.append("</responseCode>\n");

        node.append("\t\t<responseText>");
        node.append(responseText);
        node.append("</responseText>\n");
    }

    node.append("\t\t<frameId>");
    node.append(frameId);
    node.append("</frameId>\n");

    node.append("\t\t<message><![CDATA[");
    node.append(message);
    node.append("]]></message>\n");

    write(outputFileDescriptor, node.data(), node.length());
}

void getMessageData(UtlString& content, 
                   UtlBoolean isOutgoing,
                   UtlString& date,
                   UtlString& hostname,
                   UtlString& eventCount,
                   int outputFileDescriptor)
{
    UtlString remoteHost;
    UtlString remoteAddress;
    UtlString remotePort;
    UtlString remoteSourceAddress;
    UtlString message;
    UtlString branchId;
    UtlString transactionId;
    UtlString method;
    UtlString responseCode;
    UtlString responseText;

    int hostIndex = content.index("----Remote Host:");
    if(hostIndex > 0)
    {
        hostIndex += 16;
        int hostEnd = content.index("----", hostIndex);
        remoteHost.append(&(content.data()[hostIndex]),
                          hostEnd - hostIndex);

        remoteAddress = remoteHost;

        remoteHost.append(":");

        int portIndex = hostEnd + 11;
        int portEnd = content.index("----", portIndex);
        remotePort.append(&(content.data()[portIndex]),
                          portEnd - portIndex);
        remoteHost.append(remotePort);

        int messageIndex = portEnd + 5;
        unsigned int messageEnd;
        if(isOutgoing)
        {
            messageEnd = content.index("--------------------END", messageIndex);
        }
        else
        {
            messageEnd = content.index("====================END", messageIndex);
        }
        if ( UTL_NOT_FOUND == messageEnd )
        {
           messageEnd = content.length();
        }
        
        message.append(&(content.data()[messageIndex]),
                          messageEnd - messageIndex);

        SipMessage sipMsg(message);

        remoteSourceAddress = remoteHost;

        if(sipMsg.isResponse())
        {
            sipMsg.getFirstHeaderLinePart(1, &responseCode);
            sipMsg.getFirstHeaderLinePart(2, &responseText);

            if(!isOutgoing)
            {
                remoteHost.remove(0);
            }
        }
        else
        {
            sipMsg.getRequestMethod(&method);

            //We can derive the source entity from the via in
            // incoming requests
            if(!isOutgoing)
            {
                UtlString viaAddress;
                UtlString protocol;
                int viaPortNum;

                sipMsg.getLastVia(&viaAddress, 
                                  &viaPortNum, 
                                  &protocol);
                char numBuff[30];
                sprintf(numBuff, "%d", viaPortNum);
                UtlString viaPort(numBuff);

                remoteHost = remoteAddress + ":" + viaPort;

            }
        }

        // transaction token: cseq,call-id,from-tag,to=tag
        int cseq;
        UtlString cseqMethod;
        sipMsg.getCSeqField(&cseq, &cseqMethod);
        char numBuf[20];
        sprintf(numBuf, "%d", cseq);
        UtlString callId;
        sipMsg.getCallIdField(&callId);
        Url to;
        sipMsg.getToUrl(to);
        UtlString toTag;
        to.getFieldParameter("tag", toTag);
        Url from;
        sipMsg.getFromUrl(from);
        UtlString fromTag;
        from.getFieldParameter("tag", fromTag);

        transactionId = numBuf;
        transactionId.append(",");
        transactionId.append(callId);
        transactionId.append(",");
        transactionId.append(fromTag);
        transactionId.append(",");
        transactionId.append(toTag);

        // Write all the stuff out

        // Write out the node container start
        writeBranchNodeBegin(outputFileDescriptor);

        // Write out the branchId container start
        writeBranchSetBegin(outputFileDescriptor);

        // Write out the branchIds
        int viaIndex = 0;
        UtlString topVia;
        while(sipMsg.getViaField(&topVia, viaIndex))
        {
            SipMessage::getViaTag(topVia.data(), 
                                  "branch", 
                                  branchId);
            writeBranchId(outputFileDescriptor, branchId);
            viaIndex++;
        }

        // Write out the branchId container finish
        writeBranchSetEnd(outputFileDescriptor);

        // Write out the rest of the node data
        writeBranchNodeData(outputFileDescriptor, 
                 date,
                 isOutgoing ? hostname : remoteHost,
                 isOutgoing ? remoteHost : hostname,
                 isOutgoing ? hostname : remoteSourceAddress,
                 isOutgoing ? remoteSourceAddress : hostname,
                 transactionId,
                 eventCount,
                 method,
                 responseCode,
                 responseText,
                 message);

        // Write out the node container finish
        writeBranchNodeEnd(outputFileDescriptor);
    }
}

void convertToXml(UtlString& bufferString, int outputFileDescriptor)
{
    UtlString date;
    UtlString eventCount;
    UtlString facility;
    UtlString priority;
    UtlString hostname;
    UtlString taskname;
    UtlString taskId;
    UtlString processId;
    UtlString content;

    OsSysLog::parseLogString(bufferString.data(),
                             date,
                             eventCount,
                             facility,
                             priority,
                             hostname,
                             taskname,
                             taskId,
                             processId,
                             content);

    if(facility.compareTo("OUTGOING") == 0)
    {
        hostname.append("-");
        hostname.append(processId);

        getMessageData(content, 
                       TRUE,
                       date,
                       hostname,
                       eventCount,
                       outputFileDescriptor);


    }

    else if(facility.compareTo("INCOMING") == 0)
    {
        hostname.append("-");
        hostname.append(processId);

        getMessageData(content, 
                       FALSE,
                       date,
                       hostname,
                       eventCount,
                       outputFileDescriptor);

    }
}


int main(int argc, char * argv[])
{

	int i, ifd = 0, ofd = 1;
	
	for(i = 1; i < argc; i++)
	{
		if(!strcmp(argv[i], "-h"))
		{
			fprintf(stderr, "Usage:\n\t%s [-h] [if=input] [of=output]\n", argv[0]);
			return 0;
		}
		else if(!strncmp(argv[i], "if=", 3))
		{
			ifd = open(&argv[i][3], O_RDONLY);
			if(ifd == -1)
			{
				fprintf(stderr, "%s: %s\n", &argv[i][3], strerror(errno));
				return 1;
			}
		}
		else if(!strncmp(argv[i], "of=", 3))
		{
#ifdef _WIN32
			ofd = open(&argv[i][3], O_BINARY | O_WRONLY | O_CREAT, 0644);
#else
			/* No such thing as a "binary" file on POSIX */
			ofd = open(&argv[i][3], O_WRONLY | O_CREAT, 0644);
#endif
			if(ofd == -1)
			{
				fprintf(stderr, "%s: %s\n", &argv[i][3], strerror(errno));
				return 1;
			}
		}
		else
		{
			fprintf(stderr, "Unknown option: %s\n", argv[i]);
			return 1;
		}
	}
	
    writeMessageNodesBegin(ofd);

	char inputBuffer[BUFFER_SIZE + 1];
    UtlString bufferString;
    int lineLen;
    int nextLineStart;

    do
    {

    	i = read(ifd, inputBuffer, BUFFER_SIZE);

        if(i > 0) 
        {
            inputBuffer[i] = '\0';
            bufferString.append(inputBuffer);
        }

        do
        {
            lineLen = 
                NameValueTokenizer::findNextLineTerminator(bufferString.data(),
                                                       bufferString.length(),
                                                       &nextLineStart);

            // If a new line was found
            if(nextLineStart > 0)
            {
                UtlString line;
                line.append(bufferString, lineLen);
                bufferString.remove(0, nextLineStart);

                convertToXml(line, ofd);
            }
        }
        while(nextLineStart > 0);
    }
    while(i && i != -1);
	

    // Last line without a newline
    convertToXml(bufferString, ofd);
	
    writeMessageNodesEnd(ofd);

    close(ofd);

	return 0;
}
