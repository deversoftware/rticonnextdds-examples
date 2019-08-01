/*******************************************************************************
 (c) 2005-2014 Copyright, Real-Time Innovations, Inc.  All rights reserved.
 RTI grants Licensee a license to use, modify, compile, and create derivative
 works of the Software.  Licensee has the right to distribute object form only
 for use with RTI products.  The Software is provided "as is", with no warranty
 of any type, including any warranty for fitness for any purpose. RTI is under
 no obligation to maintain or support the Software.  RTI shall not be liable for
 any incidental or consequential damages arising out of the use or inability to
 use the software.
 ******************************************************************************/
/* keys_subscriber.c

A subscription example

This file is derived from code automatically generated by the rtiddsgen
command:

rtiddsgen -language C -example <arch> keys.idl

Example subscription of type keys automatically generated by
'rtiddsgen'. To test them, follow these steps:

(1) Compile this file and the example publication.

(2) Start the subscription with the command
objs/<arch>/keys_subscriber <domain_id> <sample_count>

(3) Start the publication with the command
objs/<arch>/keys_publisher <domain_id> <sample_count>

(4) [Optional] Specify the list of discovery initial peers and
multicast receive addresses via an environment variable or a file
(in the current working directory) called NDDS_DISCOVERY_PEERS.

You can run any number of publishers and subscribers programs, and can
add and remove them dynamically from the domain.


Example:

To run the example application on domain <domain_id>:

On UNIX systems:

objs/<arch>/keys_publisher <domain_id>
objs/<arch>/keys_subscriber <domain_id>

On Windows systems:

objs\<arch>\keys_publisher <domain_id>
objs\<arch>\keys_subscriber <domain_id>


modification history
------------ -------
*/

#include "keys.h"
#include "keysSupport.h"
#include "ndds/ndds_c.h"
#include <stdio.h>
#include <stdlib.h>

void keysListener_on_requested_deadline_missed(
        void *listener_data,
        DDS_DataReader *reader,
        const struct DDS_RequestedDeadlineMissedStatus *status)
{
}

void keysListener_on_requested_incompatible_qos(
        void *listener_data,
        DDS_DataReader *reader,
        const struct DDS_RequestedIncompatibleQosStatus *status)
{
}

void keysListener_on_sample_rejected(
        void *listener_data,
        DDS_DataReader *reader,
        const struct DDS_SampleRejectedStatus *status)
{
}

void keysListener_on_liveliness_changed(
        void *listener_data,
        DDS_DataReader *reader,
        const struct DDS_LivelinessChangedStatus *status)
{
}

void keysListener_on_sample_lost(
        void *listener_data,
        DDS_DataReader *reader,
        const struct DDS_SampleLostStatus *status)
{
}

void keysListener_on_subscription_matched(
        void *listener_data,
        DDS_DataReader *reader,
        const struct DDS_SubscriptionMatchedStatus *status)
{
}

/**** Start changes for Advanced_Keys ****/
/* Track instance state */
#define INSTANCE_STATE_INACTIVE 0
#define INSTANCE_STATE_ACTIVE 1
#define INSTANCE_STATE_NO_WRITERS 2
#define INSTANCE_STATE_DISPOSED 3

int states[3] = { INSTANCE_STATE_INACTIVE,
                  INSTANCE_STATE_INACTIVE,
                  INSTANCE_STATE_INACTIVE };


/* These are not called by DDS. on_data_available() calls
the appropriate function when it gets updates about
an instances' status
*/
void keysListener_new_instance_found(
        keysDataReader *keys_reader,
        const struct DDS_SampleInfo *info,
        const struct keys *msg);

void keysListener_instance_lost_writers(
        keysDataReader *keys_reader,
        const struct DDS_SampleInfo *info,
        const struct keys *msg);

void keysListener_instance_disposed(
        keysDataReader *keys_reader,
        const struct DDS_SampleInfo *info,
        const struct keys *msg);

/* Called to handle relevant data samples */
void keysListener_handle_data(
        keysDataReader *keys_reader,
        const struct DDS_SampleInfo *info,
        const struct keys *msg);

/* Called to determine if a key is relevant to this application */
int keysListener_key_is_relevant(const struct keys *msg);

void keysListener_on_data_available(void *listener_data, DDS_DataReader *reader)
{
    keysDataReader *keys_reader = NULL;
    struct keysSeq data_seq = DDS_SEQUENCE_INITIALIZER;
    struct DDS_SampleInfoSeq info_seq = DDS_SEQUENCE_INITIALIZER;
    DDS_ReturnCode_t retcode;
    int i;

    keys_reader = keysDataReader_narrow(reader);
    if (keys_reader == NULL) {
        printf("DataReader narrow error\n");
        return;
    }

    while (1) {
        /* Given DDS_HANDLE_NIL as a parameter, take_next_instance returns
        a sequence containing samples from only the next (in a well-determined
        but unspecified order) un-taken instance.
        */
        retcode = keysDataReader_take_next_instance(
                keys_reader,
                &data_seq,
                &info_seq,
                DDS_LENGTH_UNLIMITED,
                &DDS_HANDLE_NIL,
                DDS_ANY_SAMPLE_STATE,
                DDS_ANY_VIEW_STATE,
                DDS_ANY_INSTANCE_STATE);

        if (retcode == DDS_RETCODE_NO_DATA) {
            break;
        } else if (retcode != DDS_RETCODE_OK) {
            printf("read error %d\n", retcode);
            break;
        }

        /* We process all the obtained samples for a particular instance */
        for (i = 0; i < keysSeq_get_length(&data_seq); ++i) {
            struct DDS_SampleInfo *info = NULL;
            struct keys *data = NULL;

            info = DDS_SampleInfoSeq_get_reference(&info_seq, i);
            data = keysSeq_get_reference(&data_seq, i);

            /* We first check if the sample includes valid data */
            if (info->valid_data) {
                if (info->view_state == DDS_NEW_VIEW_STATE) {
                    keysListener_new_instance_found(keys_reader, info, data);
                }

                /* We check if the obtained samples are associated to one
                of the instances of interest.
                Since take_next_instance gives sequences of the same instance,
                we only need to test this for the first sample obtained.
                */
                if (i == 0 && !keysListener_key_is_relevant(data)) {
                    break;
                }

                keysListener_handle_data(keys_reader, info, data);

            } else {
                /* Since there is not valid data, it may include metadata */
                keys dummy;
                retcode = keysDataReader_get_key_value(
                        keys_reader,
                        &dummy,
                        &info->instance_handle);
                if (retcode != DDS_RETCODE_OK) {
                    printf("get_key_value error %d\n", retcode);
                    continue;
                }

                /* Here we print a message and change the instance state
                if the instance state is ALIVE_NO_WRITERS or ALIVE_DISPOSED */
                if (info->instance_state
                    == DDS_NOT_ALIVE_NO_WRITERS_INSTANCE_STATE) {
                    keysListener_instance_lost_writers(
                            keys_reader,
                            info,
                            &dummy);
                } else if (
                        info->instance_state
                        == DDS_NOT_ALIVE_DISPOSED_INSTANCE_STATE) {
                    keysListener_instance_disposed(keys_reader, info, &dummy);
                }
            }
        }

        /* Prepare sequences for next take_next_instance */
        retcode = keysDataReader_return_loan(keys_reader, &data_seq, &info_seq);
        if (retcode != DDS_RETCODE_OK) {
            printf("return loan error %d\n", retcode);
        }

        keysSeq_set_maximum(&data_seq, 0);
        DDS_SampleInfoSeq_set_maximum(&info_seq, 0);
    }
}

void keysListener_new_instance_found(
        keysDataReader *keys_reader,
        const struct DDS_SampleInfo *info,
        const struct keys *msg)
{
    /* There are really three cases here:
    1.) truly new instance
    2.) instance lost all writers, but now we're getting data again
    3.) instance was disposed, but a new one has been created

    We distinguish these cases by examining generation counts, BUT
    note that if the instance resources have been reclaimed, the
    generation counts may be reset to 0.

    Instances are eligible for resource cleanup if there are no
    active writers and all samples have been taken.  To reliably
    determine which case a 'new' instance falls into, the application
    must store state information on a per-instance basis.

    Note that this example assumes that state changes only occur via
    explicit register_instance(), unregister_instance() and dispose()
    calls from the datawriter.  In reality, these changes could also
    occur due to lost liveliness or missed deadlines, so those
    listeners would also need to update the instance state.
    */

    switch (states[msg->code]) {
    case INSTANCE_STATE_INACTIVE:
        printf("New instance found; code = %d\n", msg->code);
        break;
    case INSTANCE_STATE_ACTIVE:
        /* An active instance should never be interpreted as new */
        printf("Error, 'new' already-active instance found; code = %d\n",
               msg->code);
        break;
    case INSTANCE_STATE_NO_WRITERS:
        printf("Found writer for instance; code = %d\n", msg->code);
        break;
    case INSTANCE_STATE_DISPOSED:
        printf("Found reborn instance; code = %d\n", msg->code);
        break;
    }
    states[msg->code] = INSTANCE_STATE_ACTIVE;
}

void keysListener_instance_lost_writers(
        keysDataReader *keys_reader,
        const struct DDS_SampleInfo *info,
        const struct keys *msg)
{
    printf("Instance has no writers; code = %d\n", msg->code);
    states[msg->code] = INSTANCE_STATE_NO_WRITERS;
}

void keysListener_instance_disposed(
        keysDataReader *keys_reader,
        const struct DDS_SampleInfo *info,
        const struct keys *msg)
{
    printf("Instance disposed; code = %d\n", msg->code);
    states[msg->code] = INSTANCE_STATE_DISPOSED;
}

/* Called to handle relevant data samples */
void keysListener_handle_data(
        keysDataReader *keys_reader,
        const struct DDS_SampleInfo *info,
        const struct keys *msg)
{
    printf("code: %d, x: %d, y: %d\n", msg->code, msg->x, msg->y);
}

int keysListener_key_is_relevant(const struct keys *msg)
{
    /* For this example we just care about codes > 0,
    which are the ones related to instances ins1 and ins2 .*/
    return (msg->code > 0) ? 1 : 0;
}

/**** End changes for Advanced_Keys ****/

/* Delete all entities */
static int subscriber_shutdown(DDS_DomainParticipant *participant)
{
    DDS_ReturnCode_t retcode;
    int status = 0;

    if (participant != NULL) {
        retcode = DDS_DomainParticipant_delete_contained_entities(participant);
        if (retcode != DDS_RETCODE_OK) {
            printf("delete_contained_entities error %d\n", retcode);
            status = -1;
        }

        retcode = DDS_DomainParticipantFactory_delete_participant(
                DDS_TheParticipantFactory,
                participant);
        if (retcode != DDS_RETCODE_OK) {
            printf("delete_participant error %d\n", retcode);
            status = -1;
        }
    }

    /* RTI Connext provides the finalize_instance() method on
    domain participant factory for users who want to release memory used
    by the participant factory. Uncomment the following block of code for
    clean destruction of the singleton. */
    /*
    retcode = DDS_DomainParticipantFactory_finalize_instance();
    if (retcode != DDS_RETCODE_OK) {
    printf("finalize_instance error %d\n", retcode);
    status = -1;
    }
    */

    return status;
}

static int subscriber_main(int domainId, int sample_count)
{
    DDS_DomainParticipant *participant = NULL;
    DDS_Subscriber *subscriber = NULL;
    DDS_Topic *topic = NULL;
    struct DDS_DataReaderListener reader_listener =
            DDS_DataReaderListener_INITIALIZER;
    DDS_DataReader *reader = NULL;
    DDS_ReturnCode_t retcode;
    const char *type_name = NULL;
    int count = 0;
    struct DDS_Duration_t poll_period = { 1, 0 };

    /* If you want to set the DataReader QoS settings
     * programmatically rather than using the XML, you will need to add
     * the following line to your code
     */
    /*struct DDS_DataReaderQos datareader_qos = DDS_DataReaderQos_INITIALIZER;*/

    /* To customize participant QoS, use
    the configuration file USER_QOS_PROFILES.xml */
    participant = DDS_DomainParticipantFactory_create_participant(
            DDS_TheParticipantFactory,
            domainId,
            &DDS_PARTICIPANT_QOS_DEFAULT,
            NULL /* listener */,
            DDS_STATUS_MASK_NONE);
    if (participant == NULL) {
        printf("create_participant error\n");
        subscriber_shutdown(participant);
        return -1;
    }

    /* To customize subscriber QoS, use
    the configuration file USER_QOS_PROFILES.xml */
    subscriber = DDS_DomainParticipant_create_subscriber(
            participant,
            &DDS_SUBSCRIBER_QOS_DEFAULT,
            NULL /* listener */,
            DDS_STATUS_MASK_NONE);
    if (subscriber == NULL) {
        printf("create_subscriber error\n");
        subscriber_shutdown(participant);
        return -1;
    }

    /* Register the type before creating the topic */
    type_name = keysTypeSupport_get_type_name();
    retcode = keysTypeSupport_register_type(participant, type_name);
    if (retcode != DDS_RETCODE_OK) {
        printf("register_type error %d\n", retcode);
        subscriber_shutdown(participant);
        return -1;
    }

    /* To customize topic QoS, use
    the configuration file USER_QOS_PROFILES.xml */
    topic = DDS_DomainParticipant_create_topic(
            participant,
            "Example keys",
            type_name,
            &DDS_TOPIC_QOS_DEFAULT,
            NULL /* listener */,
            DDS_STATUS_MASK_NONE);
    if (topic == NULL) {
        printf("create_topic error\n");
        subscriber_shutdown(participant);
        return -1;
    }

    /* Set up a data reader listener */
    reader_listener.on_requested_deadline_missed =
            keysListener_on_requested_deadline_missed;
    reader_listener.on_requested_incompatible_qos =
            keysListener_on_requested_incompatible_qos;
    reader_listener.on_sample_rejected = keysListener_on_sample_rejected;
    reader_listener.on_liveliness_changed = keysListener_on_liveliness_changed;
    reader_listener.on_sample_lost = keysListener_on_sample_lost;
    reader_listener.on_subscription_matched =
            keysListener_on_subscription_matched;
    reader_listener.on_data_available = keysListener_on_data_available;

    /* To customize data reader QoS, use
    the configuration file USER_QOS_PROFILES.xml */
    reader = DDS_Subscriber_create_datareader(
            subscriber,
            DDS_Topic_as_topicdescription(topic),
            &DDS_DATAREADER_QOS_DEFAULT,
            &reader_listener,
            DDS_STATUS_MASK_ALL);

    /* If you want to set the writer_data_lifecycle QoS settings
     * programmatically rather than using the XML, you will need to add
     * the following lines to your code and comment out the create_datareader
     * call above.
     */

    /*
    retcode = DDS_Subscriber_get_default_datareader_qos(subscriber,
    &datareader_qos); if (retcode != DDS_RETCODE_OK) {
    printf("get_default_datareader_qos error\n");
    return -1;
    }

    datareader_qos.ownership.kind = DDS_EXCLUSIVE_OWNERSHIP_QOS;

    reader = DDS_Subscriber_create_datareader(
    subscriber, DDS_Topic_as_topicdescription(topic),
    &datareader_qos, &reader_listener, DDS_STATUS_MASK_ALL);
    */

    if (reader == NULL) {
        printf("create_datareader error\n");
        subscriber_shutdown(participant);
        return -1;
    }

    /* Main loop */
    for (count = 0; (sample_count == 0) || (count < sample_count); ++count) {
        // printf("keys subscriber sleeping for %d sec...\n",poll_period.sec);
        NDDS_Utility_sleep(&poll_period);
    }

    /* Cleanup and delete all entities */
    return subscriber_shutdown(participant);
}

#if defined(RTI_WINCE)
int wmain(int argc, wchar_t **argv)
{
    int domainId = 0;
    int sample_count = 0; /* infinite loop */

    if (argc >= 2) {
        domainId = _wtoi(argv[1]);
    }
    if (argc >= 3) {
        sample_count = _wtoi(argv[2]);
    }

    /* Uncomment this to turn on additional logging
    NDDS_Config_Logger_set_verbosity_by_category(
    NDDS_Config_Logger_get_instance(),
    NDDS_CONFIG_LOG_CATEGORY_API,
    NDDS_CONFIG_LOG_VERBOSITY_STATUS_ALL);
    */

    return subscriber_main(domainId, sample_count);
}
#elif !(defined(RTI_VXWORKS) && !defined(__RTP__)) && !defined(RTI_PSOS)
int main(int argc, char *argv[])
{
    int domainId = 0;
    int sample_count = 0; /* infinite loop */

    if (argc >= 2) {
        domainId = atoi(argv[1]);
    }
    if (argc >= 3) {
        sample_count = atoi(argv[2]);
    }

    /* Uncomment this to turn on additional logging
    NDDS_Config_Logger_set_verbosity_by_category(
    NDDS_Config_Logger_get_instance(),
    NDDS_CONFIG_LOG_CATEGORY_API,
    NDDS_CONFIG_LOG_VERBOSITY_STATUS_ALL);
    */

    return subscriber_main(domainId, sample_count);
}
#endif

#ifdef RTI_VX653
const unsigned char *__ctype = NULL;

void usrAppInit()
{
    #ifdef USER_APPL_INIT
    USER_APPL_INIT; /* for backwards compatibility */
    #endif

    /* add application specific code here */
    taskSpawn(
            "sub",
            RTI_OSAPI_THREAD_PRIORITY_NORMAL,
            0x8,
            0x150000,
            (FUNCPTR) subscriber_main,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0);
}
#endif
