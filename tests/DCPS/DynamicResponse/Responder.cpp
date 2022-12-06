#include "Common.h"

#include <tests/Utils/StatusMatching.h>

#include <dds/DCPS/XTypes/DynamicTypeSupport.h>
#include <dds/DCPS/XTypes/DynamicDataFactory.h>

#include <dds/DdsDcpsCoreTypeSupportC.h>

#include <string>

struct DynamicTopic {
  DynamicTopic()
  : found(false)
  , get_dynamic_type_rc(DDS::RETCODE_OK)
  {
  }

  bool found;
  DDS::ReturnCode_t get_dynamic_type_rc;
  DDS::DynamicType_var type;
  DDS::TypeSupport_var ts;
  DDS::Topic_var topic;
  DDS::DynamicDataWriter_var writer;
};

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  using OpenDDS::DCPS::retcode_to_string;

  Test t("responder");
  if (!t.init(argc, argv)) {
    return 1;
  }

  Topic<DDS::PublicationBuiltinTopicData> pub_bit(t);
  if (!pub_bit.init_bit(OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC)) {
    return 1;
  }

  typedef std::map<std::string, DynamicTopic> DynamicTopicMap;
  DynamicTopicMap topics;
  topics["SimpleStruct"] = DynamicTopic();
  topics["SimpleUnion"] = DynamicTopic();
  topics["SimpleStructNotComplete"].get_dynamic_type_rc = DDS::RETCODE_NO_DATA;

  t.wait_for("origin", "ready");

  DDS::PublicationBuiltinTopicDataSeq pub_bit_seq;
  if (!pub_bit.read_multiple(pub_bit_seq)) {
    return 1;
  }
  for (unsigned i = 0; i < pub_bit_seq.length(); ++i) {
    DDS::PublicationBuiltinTopicData& pb = pub_bit_seq[i];
    ACE_DEBUG((LM_DEBUG, "Learned about topic \"%C\" type \"%C\"\n",
      pb.topic_name.in(), pb.type_name.in()));

    DynamicTopicMap::iterator it = topics.find(pb.topic_name.in());
    if (it == topics.end()) {
      ACE_ERROR((LM_ERROR, "ERROR: Unexpected topic %C\n", pb.topic_name.in()));
    } else {
      DynamicTopic& topic = it->second;
      const DDS::ReturnCode_t rc =
        TheServiceParticipant->get_dynamic_type(topic.type, t.dp, pb.key);
      if (rc != topic.get_dynamic_type_rc) {
        ACE_ERROR((LM_ERROR, "ERROR: Expected %C, but got %C\n",
          retcode_to_string(topic.get_dynamic_type_rc), retcode_to_string(rc)));
        t.exit_status = 1;
      }
      if (rc != DDS::RETCODE_OK) {
        continue;
      }

      topic.ts = new DDS::DynamicTypeSupport(topic.type);
      if (!t.init_topic(topic.ts, topic.topic)) {
        t.exit_status = 1;
        continue;
      }

      topic.writer = t.create_writer<DDS::DynamicDataWriter>(topic.topic);

      DDS::DataWriter_var dw = DDS::DataWriter::_duplicate(topic.writer);
      if (Utils::wait_match(dw, 1, Utils::EQ)) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): Error waiting for match for dw\n"));
        continue;
      }
    }
  }

  for (DynamicTopicMap::iterator it = topics.begin(); it != topics.end(); ++it) {
    DynamicTopic& topic = it->second;
    if (topic.type) {
      // TODO: Read First

      DDS::DynamicData_var dd = DDS::DynamicDataFactory::get_instance()->create_data(topic.type);
      const DDS::TypeKind tk = topic.type->get_kind();
      if (!t.check_rc(dd->set_string_value(0,
          tk == OpenDDS::XTypes::TK_STRUCTURE ? "Hello struct" : "Hello union"),
          "set_string_value failed")) {
        t.exit_status = 1;
        continue;
      }
      if (!t.check_rc(topic.writer->write(dd, DDS::HANDLE_NIL), "write failed")) {
        t.exit_status = 1;
      }
    }
  }

  t.post("done");

  t.wait_for("origin", "done");

  return t.exit_status;
}
