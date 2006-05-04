#
# Copyright (C) 2006 SIPfoundry Inc.
# Licensed by SIPfoundry under the LGPL license.
# 
# Copyright (C) 2006 Pingtel Corp.
# Licensed to SIPfoundry under a Contributor Agreement.
#
##############################################################################

$SOURCE_DIR = File.dirname(__FILE__)    # directory in which this file is located

# system requirements
require 'parsedate'
require File.join($SOURCE_DIR, '..', 'test_helper')

# application requirements
require File.join($SOURCE_DIR, '..', '..', 'call_resolver')


# :TODO: Make it easy to run all the unit tests, possibly via Rakefile, for build loop.
class CallResolverTest < Test::Unit::TestCase
  fixtures :call_state_events, :cdrs
  
  TEST_AOR = 'aor'
  TEST_CONTACT = 'contact'
  TEST_CALL_ID = 'call ID'
  TEST_FROM_TAG = 'f'
  TEST_TO_TAG = 't'
 
  SECONDS_IN_A_DAY = 24 * 60 * 60
  
  CALL_ID1 = 'call_id1'
  CALL_ID2 = 'call_id2'
  CALL_ID3 = 'call_id3'
  
  TEST_DB1 = 'SIPXCSE_TEST1'
  TEST_DB2 = 'SIPXCSE_TEST2'
 
  LOCALHOST = 'localhost'
 
public

  def setup
    super
    
    # Create the CallResolver, giving it the location of the test config file.
    @resolver = CallResolver.new(File.join($SOURCE_DIR, 'data/callresolver-config'))
  end

  def test_connect_to_cdr_database
    # Ensure that we are disconnected first
    if ActiveRecord::Base.connected?
      ActiveRecord::Base.remove_connection
    end
    assert(!ActiveRecord::Base.connected?, 'Must be disconnected from database')
    
    # Connect to the database and verify that we are connected.
    # Rails quirk: we can't assert "connected?" because the connection has been
    # established but is not active yet, so "connected?" is not true.  But we
    # can check that the connection exists.
    @resolver.send(:connect_to_cdr_database)
    assert(ActiveRecord::Base.connection, 'Must be connected to database')
  end

  def test_load_distrib_events_in_time_window
    # Create the two test databases if they don't already exist.
    # Note: using existing databases to save time may fail if the schema changes
    # in, which case you should manually delete the databases and run the test
    # again.
    DatabaseUtils.create_cdr_database(TEST_DB1)
    DatabaseUtils.create_cdr_database(TEST_DB2)
    
    cse_url1 = DatabaseUrl.new(TEST_DB1)
    cse_url2 = DatabaseUrl.new(TEST_DB2)
    
    # Pick an arbitrary base event time for all events
    start_time = Time.now
    
    # Test loading the events.  Restore the CallStateEvent DB connection after
    # the test if there was one.  Restore the CSE database URLs.
    conn = CallStateEvent.remove_connection
    cse_database_urls = @resolver.send(:cse_database_urls)
    begin
      # Put events for the first call and part of the second call into the first
      # test DB.
      # Note: only the time values on the call that is split across two DBs
      # matter, because only that call will be time-sorted when the events get
      # merged.
      CallStateEvent.establish_connection(DatabaseUrl.new(TEST_DB1).to_hash)
      CallStateEvent.destroy_all
      e1_1 = create_test_cse(CALL_ID1, start_time)
      e1_2 = create_test_cse(CALL_ID1, start_time)
      e2_1 = create_test_cse(CALL_ID2, start_time + 1)
      e2_3 = create_test_cse(CALL_ID2, start_time + 3)
      
      # Put events for the rest of the second call and the third call into the
      # second test DB.
      CallStateEvent.establish_connection(DatabaseUrl.new(TEST_DB2).to_hash)
      CallStateEvent.destroy_all
      e2_2 = create_test_cse(CALL_ID2, start_time + 2)
      e2_4 = create_test_cse(CALL_ID2, start_time + 4)
      e3_1 = create_test_cse(CALL_ID3, start_time)
      
      call1 = [e1_1, e1_2]
      call2_part1 = [e2_1, e2_3]
      call2_part2 = [e2_2, e2_4]
      call2 = (call2_part1 + call2_part2).sort!{|x, y| x.event_time <=> y.event_time}
      call3 = [e3_1]
      all_calls = [[call1, call2_part1],           # call arrays for first DB
                   [call2_part2, call3]]           # call arrays for second DB
      
      end_time = start_time + 10
      @resolver.config.send(:cse_database_urls=, [cse_url1, cse_url2])
      call_map = @resolver.send(:load_distrib_events_in_time_window, start_time, end_time)
      expected_call_map_entries = [call1, call2, call3]
      check_call_map(all_calls, call_map, expected_call_map_entries)
    
    ensure
      # Restore the original connection, or at least clear the test connection
      if conn
        CallStateEvent.establish_connection(conn)
      else
        CallStateEvent.remove_connection
      end
      
      # Restore original DB URLs
      @resolver.config.send(:cse_database_urls=, cse_database_urls)
    end
  end

  # Create a test CSE.  Fill in dummy values for fields we don't care about but
  # have to be filled in because of not-null DB constraints
  def create_test_cse(call_id, event_time)
    CallStateEvent.create(:observer => 'observer',
                          :event_seq => 0,
                          :event_time => event_time,
                          :event_type => CallStateEvent::CALL_REQUEST_TYPE,
                          :cseq => 0,
                          :call_id => call_id,
                          :from_url => 'from_url',
                          :to_url => 'to_url')    
  end

  def test_merge_events_for_call    
    e1 = CallStateEvent.new(:call_id => CALL_ID1)
    e2 = CallStateEvent.new(:call_id => CALL_ID1)
    
    time = Time.now;
    e3 = CallStateEvent.new(:call_id => CALL_ID2, :event_time => time)
    e4 = CallStateEvent.new(:call_id => CALL_ID2, :event_time => time + 1)
    e5 = CallStateEvent.new(:call_id => CALL_ID2, :event_time => time + 2)
    e6 = CallStateEvent.new(:call_id => CALL_ID2, :event_time => time + 3)
    
    e7 = CallStateEvent.new(:call_id => CALL_ID3)
    
    call1 = [e1, e2]
    call2_part1 = [e3, e5]
    call2_part2 = [e4, e6]
    call2 = [e3, e4, e5, e6]
    call3 = [e7]
    all_calls = [[call1, call2_part1],           # call arrays for first DB
                 [call2_part2, call3]]           # call arrays for second DB
    call_map = {}
    @resolver.send(:merge_events_for_call, all_calls, call_map)
    expected_call_map_entries = [call1, call2, call3]
    check_call_map(all_calls, call_map, expected_call_map_entries)
  end

  def check_call_map(all_calls, call_map, expected_call_map_entries)
    assert_equal(expected_call_map_entries.size, call_map.size)
    [CALL_ID1, CALL_ID2, CALL_ID3].each_with_index do |call_id, i|
      assert_equal(expected_call_map_entries[i], call_map[call_id])
    end
  end

  def test_split_events_by_call
    call1 = 'call1'
    call2 = 'call2'
    call3 = 'call3'
    e1 = CallStateEvent.new(:call_id => call1)
    e2 = CallStateEvent.new(:call_id => call2)
    e3 = CallStateEvent.new(:call_id => call2)
    e4 = CallStateEvent.new(:call_id => call3)
    e5 = CallStateEvent.new(:call_id => call3)
    e6 = CallStateEvent.new(:call_id => call3)
    events = [e1, e2, e3, e4, e5, e6]
    spl = @resolver.send(:split_events_by_call, events)
    assert_equal([e1], spl[0])
    assert_equal([e2, e3], spl[1])
    assert_equal([e4, e5, e6], spl[2])
    
    # check empty events array as input
    assert_equal([], @resolver.send(:split_events_by_call, []))
  end

  def test_load_call_ids
    start_time = Time.parse('1990-05-17T19:30:00.000Z')
    end_time = Time.parse('1990-05-17T19:45:00.000Z')

    # Load call IDs.  Do a low level message send to bypass access control on 
    # this private method.
    call_ids = @resolver.send(:load_call_ids, start_time, end_time)
    
    # call IDs can come back in any order, so sort them to guarantee order
    call_ids.sort!
    
    # verify results
    assert_equal(3, call_ids.length, 'Wrong number of call IDs')
    assert_equal('testSimpleSuccess',
                 call_ids[0],
                 'Wrong first call ID')
    assert_equal('testSimpleSuccessBogusCallInTimeWindow',
                 call_ids[1],
                 'Wrong second call ID')
  end
  
  def test_load_events_in_time_window
    start_time = Time.parse('1990-05-17T19:30:00.000Z')
    end_time = Time.parse('1990-05-17T19:45:00.000Z')
    events = @resolver.send(:load_events_in_time_window, start_time, end_time)
    assert_equal(6, events.length, 'Wrong number of events')
    assert_equal('testSimpleSuccess', events[0].call_id, 'Wrong first call ID')
    assert_equal('testSimpleSuccess_CalleeEnd',
                 events[5].call_id,
                 'Wrong last call ID')
    assert_equal(1, events[0].cseq, 'Wrong first cseq')
    assert_equal(2, events[1].cseq, 'Wrong second cseq')
    assert_equal(3, events[2].cseq, 'Wrong third cseq')
  end
  
  def test_load_events_with_call_id
    events = load_simple_success_events
    assert_equal(3, events.length)
    events.each_index do |index|
      assert_equal(events[index].event_seq, index + 1, 'Events are not in the right order')
      assert_equal('testSimpleSuccess', events[index].call_id)
    end
  end
  
  def test_find_call_request
    assert_nil(find_call_request([]),
               "No events => must not find call request event")    
    
    # Create call request events.  An original call request has no to_tag.
    orig_req = new_call_request(Time.parse('2001-1-1'))
    req1 = new_call_request(Time.parse('2002-1-1'), 'req1')
    req2 = new_call_request(Time.parse('2003-1-1'), 'req2')
  
    # Create setup and end events  
    setup = CallStateEvent.new(:event_type => CallStateEvent::CALL_SETUP_TYPE)
    call_end = CallStateEvent.new(:event_type => CallStateEvent::CALL_END_TYPE)
  
    assert_nil(find_call_request([setup, call_end]),
               "No call request events => must not find call request event")   
    
    # When there are multiple call requests with no originals, we pick the first
    # one in the array, under the assumption that the caller has already sorted
    # the array by time.
    req = find_call_request([setup, req1, call_end, req2])
    assert_equal(req1, req,
                 'Multiple call requests with no originals => first one must win')
    
    req = find_call_request([req1, orig_req, req2])
    assert_equal(orig_req, req,
                 'Original call requests must be selected before call requests with to_tags')
    
    orig_req2 = new_call_request(Time.parse('2000-1-1'))
    req = find_call_request([call_end, orig_req2, orig_req, setup])
    assert_equal(orig_req2, req,
                 'Multiple original call requests => first one must win')
  end
  
  def new_call_request(event_time, to_tag = nil)
    params = {:event_type => CallStateEvent::CALL_REQUEST_TYPE,
              :event_time => event_time}
    params[:to_tag] = to_tag if to_tag
    CallStateEvent.new(params)
  end
  
  def find_call_request(events)
    @resolver.send(:find_call_request, events)
  end
  
  def test_start_cdr
    cdr = @resolver.send(:start_cdr,
                         call_state_events('testSimpleSuccess_1'))
    # verify the caller
    assert_equal('sip:alice@example.com', cdr.caller_aor)
    assert_equal('sip:alice@1.1.1.1', cdr.caller_contact)
    
    # verify the callee: we have the AOR but not the contact
    assert_equal('sip:bob@example.com', cdr.callee_aor)
    assert_nil(cdr.callee_contact)

    # verify the CDR
    assert_equal('testSimpleSuccess', cdr.call_id)
    assert_equal('f', cdr.from_tag)
    assert_nil(cdr.to_tag)    # don't have the to tag yet
    assert_equal(Time.parse('1990-05-17T19:30:00.000Z'), cdr.start_time)
    assert_equal(Cdr::CALL_REQUESTED_TERM, cdr.termination)
  end

  def test_best_call_leg
    events = load_simple_success_events
     
    # Pick the call leg with the best outcome and longest duration to be the
    # basis for the CDR.
    to_tag = @resolver.send(:best_call_leg, events)
    assert_equal('t', to_tag, 'Wrong to_tag for best call leg')
    
    # load events for the complicated case
    call_id = 'testComplicatedSuccess'
    events = @resolver.send(:load_events_with_call_id, call_id)
     
    to_tag = @resolver.send(:best_call_leg, events)
    assert_equal('t2', to_tag, 'Wrong to_tag for best call leg')
    
    # try again, drop the final call_end event
    to_tag = @resolver.send(:best_call_leg, events[0..4])
    assert_equal('t1', to_tag, 'Wrong to_tag for best call leg')
    
    # try again with three events
    to_tag = @resolver.send(:best_call_leg, events[0..2])
    assert_equal('t0', to_tag, 'Wrong to_tag for best call leg')
    
    # try again with two events
    to_tag = @resolver.send(:best_call_leg, events[0..1])
    assert_equal('t0', to_tag, 'Wrong to_tag for best call leg')
    
    # try again with just the call request
    to_tag = @resolver.send(:best_call_leg, events[0..0])
    assert_nil(to_tag, 'Wrong to_tag for best call leg')
  end

  def test_finish_cdr
    events = load_simple_success_events
    
    # fill in cdr_data with info from the events
    to_tag = 't'
    cdr = Cdr.new
    status = @resolver.send(:finish_cdr, cdr, events, to_tag)
    assert_equal(true, status)
    
    # Check that the CDR is filled in as expected.  It will only be partially
    # filled in because we are testing just one part of the process.
    assert_equal(to_tag, cdr.to_tag, 'Wrong to_tag')
    assert_equal(Time.parse('1990-05-17T19:31:00.000Z'), cdr.connect_time,
                            'Wrong connect_time')
    assert_equal(Time.parse('1990-05-17T19:40:00.000Z'), cdr.end_time,
                            'Wrong end_time')
    assert_equal('sip:bob@2.2.2.2', cdr.callee_contact, 'Wrong callee contact')
    assert_equal(Cdr::CALL_COMPLETED_TERM, cdr.termination, 'Wrong termination code')
    assert_nil(cdr.failure_status)
    assert_nil(cdr.failure_reason)
    
    # Test a failed call.  Check only that the failure info has been filled in
    # properly.  We've checked other info in the case above.
    # This set of events has call request, call setup, call failed.
    call_id = 'testFailed'
    events = @resolver.send(:load_events_with_call_id, call_id)
    check_failed_call(events, to_tag)
    
    # Try again without the call setup event.
    events.delete_if {|event| event.call_setup?}
    check_failed_call(events, to_tag)
  end
  
  def test_finish_cdr_callee_hangs_up
    events = load_simple_success_events_callee_hangs_up
    
    # fill in cdr_data with info from the events
    to_tag = 't'
    cdr = Cdr.new
    status = @resolver.send(:finish_cdr, cdr, events, to_tag)
    assert_equal(true, status)
    
    # Check that the CDR is filled in as expected.  It will only be partially
    # filled in because we are testing just one part of the process.
    assert_equal(to_tag, cdr.to_tag, 'Wrong to_tag')
    assert_equal(Time.parse('1990-05-17T19:41:00.000Z'), cdr.connect_time,
                            'Wrong connect_time')
    assert_equal(Time.parse('1990-05-17T19:50:00.000Z'), cdr.end_time,
                            'Wrong end_time')
    assert_equal('sip:bob@2.2.2.2', cdr.callee_contact, 'Wrong callee contact')
    assert_equal(Cdr::CALL_COMPLETED_TERM, cdr.termination, 'Wrong termination code')
    assert_nil(cdr.failure_status)
    assert_nil(cdr.failure_reason)
  end

  # Helper method for test_finish_cdr.  Check that failure info has been filled
  # in properly.
  def check_failed_call(events, to_tag)
    cdr = Cdr.new
    status = @resolver.send(:finish_cdr, cdr, events, to_tag)
    assert_equal(true, status, 'Finishing the CDR failed')
    assert_equal(Cdr::CALL_FAILED_TERM, cdr.termination, 'Wrong termination code')
    assert_equal(499, cdr.failure_status)
    assert_equal("You Can't Always Get What You Want", cdr.failure_reason) 
  end
  
  def test_save_cdr
    # For a clean test, make sure there is no preexisting CDR with the call
    # ID that we are using.
    Cdr.delete_all("call_id = '#{TEST_CALL_ID}'")
    
    # Create a new complete CDR.  Fill in mandatory fields so we don't get
    # database integrity exceptions on save. 
    cdr = Cdr.new(:call_id =>     TEST_CALL_ID,
                  :from_tag =>    TEST_FROM_TAG,
                  :to_tag =>      TEST_TO_TAG,
                  :caller_aor =>  'colbert@example.com',
                  :callee_aor =>  'report@example.com',
                  :termination => Cdr::CALL_REQUESTED_TERM)
    
    # Try to save it and confirm that it was saved.  Use a clone so we don't
    # modify the original and can reuse it.
    saved_cdr = @resolver.send(:save_cdr, cdr.clone)
    assert(saved_cdr.id, 'ID of object saved to database must be non-nil')
    
    # Try to save another clone, marked as complete.  Because the saved CDR
    # was incomplete based on its termination, the save should succeed.
    cdr2 = cdr.clone
    cdr2.termination = Cdr::CALL_COMPLETED_TERM
    save_again_cdr = @resolver.send(:save_cdr, cdr2)
    assert_not_equal(saved_cdr.id, save_again_cdr.id)
    assert_equal(Cdr::CALL_COMPLETED_TERM, save_again_cdr.termination)
    
    # Try to save another clone. Should fail because the saved CDR is
    # incomplete.  Tweak the termination to help verify this.
    cdr3 = cdr.clone
    cdr3.termination = Cdr::CALL_IN_PROGRESS_TERM
    saved_cdr = @resolver.send(:save_cdr, cdr3)
    assert_equal(save_again_cdr.id, saved_cdr.id)
    assert_equal(cdr2.termination, saved_cdr.termination)    
  end
  
  def test_find_cdr
    # Find a CDR we know is in the DB
    cdr_to_find = Cdr.new(:call_id => 'call1')
    cdr = @resolver.send(:find_cdr, cdr_to_find)
    assert(cdr, "Couldn't find CDR")
    
    # Try to find a CDR we know is not in the DB
    cdr_to_find.call_id = 'extra_bogus_call_id'
    cdr = @resolver.send(:find_cdr, cdr_to_find)
    assert_nil(cdr, "Found a CDR that shouldn't exist")
    
    # Trigger an ArgumentError
    cdr_to_find.call_id = nil
    assert_raise(ArgumentError) {cdr = @resolver.send(:find_cdr, cdr_to_find)}
  end
  
  def test_resolve_call
    ['testSimpleSuccess', 'testComplicatedSuccess', 'testFailed'].each do |call_id|
      events = @resolver.send(:load_events_with_call_id, call_id)
      @resolver.send(:resolve_call, events)
      cdr = Cdr.find_by_call_id(call_id)
      assert_not_nil(cdr, 'CDR was not created')
    end
  end
    
  def test_resolve
    Cdr.delete_all
    start_time = Time.parse('1990-01-1T000:00:00.000Z')
    end_time = Time.parse('2000-12-31T00:00.000Z')
    @resolver.resolve(start_time, end_time)
    assert_equal(4, Cdr.count, 'Wrong number of CDRs')
  end

  #-----------------------------------------------------------------------------
  # Helper methods
  
  # load and return events for the simple case
  def load_simple_success_events
    call_id = 'testSimpleSuccess'
    @resolver.send(:load_events_with_call_id, call_id)
  end
 
  # load and return events for the simple case
  def load_simple_success_events_callee_hangs_up
    call_id = 'testSimpleSuccess_CalleeEnd'
    @resolver.send(:load_events_with_call_id, call_id)
  end  
  
end
