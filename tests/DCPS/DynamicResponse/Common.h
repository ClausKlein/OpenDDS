#include <tests/Utils/DistributedConditionSet.h>
#include <tests/Utils/WaitForSample.h>

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/DCPS_Utils.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>
#include <dds/DCPS/transport/framework/TransportConfig.h>
#include <dds/DCPS/transport/framework/TransportInst.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>

#include <string>

struct Test {
  const char* const name;
  int exit_status;
  DistributedConditionSet_rch dcs;
  DDS::DomainParticipant_var dp;
  DDS::Subscriber_var sub;
  DDS::Publisher_var pub;

  Test(const char* name)
    : name(name)
    , exit_status(0)
    , dcs(OpenDDS::DCPS::make_rch<FileBasedDistributedConditionSet>())
  {
  }

  ~Test()
  {
    ACE_DEBUG((LM_DEBUG, "%C (%P|%t) shutting down...\n", name));

    dp->delete_contained_entities();
    TheParticipantFactory->delete_participant(dp);
    TheServiceParticipant->shutdown();

    ACE_DEBUG((LM_DEBUG, "%C (%P|%t) done\n", name));
    post("done");
  }

  bool init(int& argc, ACE_TCHAR* argv[])
  {
    using namespace OpenDDS::DCPS;
    using namespace OpenDDS::RTPS;

    DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

    const DDS::DomainId_t domain = 12;

    RtpsDiscovery_rch disc = make_rch<RtpsDiscovery>("rtps_disc");
    disc->use_xtypes(RtpsDiscoveryConfig::XTYPES_COMPLETE);
    TheServiceParticipant->add_discovery(static_rchandle_cast<Discovery>(disc));
    TheServiceParticipant->set_repo_domain(domain, disc->key());

    TransportConfig_rch transport_config =
      TheTransportRegistry->create_config("default_rtps_transport_config");
    TransportInst_rch transport_inst =
      TheTransportRegistry->create_inst("default_rtps_transport", "rtps_udp");
    transport_config->instances_.push_back(transport_inst);
    TheTransportRegistry->global_config(transport_config);

    dp = dpf->create_participant(domain, PARTICIPANT_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);
    if (!dp) {
      ACE_ERROR((LM_ERROR, "%C (%P|%t) ERROR: create_participant failed!\n", name));
      return false;
    }

    sub = dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);
    if (!sub) {
      ACE_ERROR((LM_ERROR, "%C (%P|%t) ERROR: create_publisher failed!\n", name));
      return false;
    }

    pub = dp->create_publisher(PUBLISHER_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);
    if (!pub) {
      ACE_ERROR((LM_ERROR, "%C (%P|%t) ERROR: pub: create_publisher failed!\n", name));
      return false;
    }

    return true;
  }

  bool init_topic(DDS::TypeSupport_var& ts, DDS::Topic_var& topic)
  {
    if (!check_rc(ts->register_type(dp, ""), "register_type")) {
      return false;
    }

    CORBA::String_var type_name = ts->get_type_name();
    topic = dp->create_topic(type_name, type_name, TOPIC_QOS_DEFAULT, 0,
      OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!topic) {
      ACE_ERROR((LM_ERROR, "%C (%P|%t) ERROR: create_topic failed!\n", name));
      return false;
    }

    return true;
  }

  template <typename DataWriterType>
  typename DataWriterType::_var_type create_writer(DDS::Topic_var& topic)
  {
    DDS::DataWriterQos qos;
    pub->get_default_datawriter_qos(qos);
    qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
    qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
    DDS::DataWriter_var dw =
      pub->create_datawriter(topic, qos, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    typename DataWriterType::_var_type dw2 = DataWriterType::_narrow(dw);
    if (!dw2) {
      ACE_ERROR((LM_ERROR, "%C (%P|%t) ERROR: create_datawriter failed!\n", name));
      exit_status = 1;
    }
    return dw2;
  }

  bool check_rc(DDS::ReturnCode_t rc, const std::string& what)
  {
    if (rc != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "%C (%P|%t) ERROR: %C: %C\n",
        name, what.c_str(), OpenDDS::DCPS::retcode_to_string(rc)));
      exit_status = 1;
      return false;
    }
    return true;
  }

  void wait_for(const std::string& actor, const std::string& what)
  {
    ACE_DEBUG((LM_DEBUG, "%C is waiting for %C to post %C\n", name, actor.c_str(), what.c_str()));
    dcs->wait_for(name, actor, what);
    ACE_DEBUG((LM_DEBUG, "%C is DONE waiting for %C to post %C\n", name, actor.c_str(), what.c_str()));
  }

  void post(const std::string& what)
  {
    ACE_DEBUG((LM_DEBUG, "%C is posting %C\n", name, what.c_str()));
    dcs->post(name, what);
  }
};

template <typename TopicType>
struct Topic {
  typedef OpenDDS::DCPS::DDSTraits<TopicType> Traits;

  Test& t;
  DDS::TypeSupport_var ts;
  DDS::Topic_var topic;
  typedef typename Traits::DataWriterType DataWriterType;
  typename DataWriterType::_var_type writer;
  typedef typename Traits::DataReaderType DataReaderType;
  typename DataReaderType::_var_type reader;
  typedef typename Traits::MessageSequenceType SeqType;

  Topic(Test& t): t(t) {}

  bool init()
  {
    using OpenDDS::DCPS::DEFAULT_STATUS_MASK;

    ts = new typename Traits::TypeSupportImplType();
    if (!t.init_topic(ts, topic)) {
      return false;
    }

    DDS::DataReader_var dr =
      t.sub->create_datareader(topic, DATAREADER_QOS_DEFAULT, 0 ,DEFAULT_STATUS_MASK);
    reader = DataReaderType::_narrow(dr);
    if (!reader) {
      ACE_ERROR((LM_ERROR, "%C (%P|%t) ERROR: create_datareader failed!\n", t.name));
      return false;
    }

    writer = t.create_writer<DataWriterType>(topic);

    return writer;
  }

  bool init_bit(const char* name)
  {
    DDS::Subscriber_var bit_subscriber = t.dp->get_builtin_subscriber();
    DDS::DataReader_var dr = bit_subscriber->lookup_datareader(name);
    reader = DataReaderType::_narrow(dr);
    if (!reader) {
      ACE_ERROR((LM_ERROR, "%C (%P|%t) failed to get %C datareader\n", name));
      return false;
    }
    return true;
  }

  bool read_multiple(SeqType& seq)
  {
    DDS::DataReader_var dr = DDS::DataReader::_duplicate(reader);
    Utils::waitForSample(dr);
    DDS::SampleInfoSeq info;
    return t.check_rc(reader->read(seq, info, DDS::LENGTH_UNLIMITED,
        DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE), "read failed");
  }

  bool read_one(TopicType& msg)
  {
    DDS::DataReader_var dr = DDS::DataReader::_duplicate(reader);
    Utils::waitForSample(dr);
    DDS::SampleInfo info;
    return t.check_rc(reader->read_next_sample(msg, info), "read failed");
  }
};
