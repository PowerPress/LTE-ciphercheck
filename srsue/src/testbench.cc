#include "srsue/hdr/testbench.h"

namespace srsue {
testbench::testbench(srslte::log_filter* _log) : current_testcase_id(0), log(_log) {}
testbench::~testbench()
{
  log->debug("DTOR called\n");
  const std::lock_guard<std::mutex> lock(testcases_mutex);
  for (auto t : testcases) {
    delete t.second;
  }
}

bool testbench::set_pcap(std::string nas_pcap, std::string mac_pcap) {
  const std::lock_guard<std::mutex> lock(testcases_mutex);

  if (testcases.find(current_testcase_id) == testcases.end()) {
    log->error("set_pcap: no testcase %i available.\n", current_testcase_id);
    return false;
  }
  auto tc = testcases[current_testcase_id];
  if (tc != nullptr) {
    tc->set_pcap(nas_pcap, mac_pcap);
    return true;
  }
  return false;
}

uint testbench::start_testcase(uint8_t _eia_mask, uint8_t _eea_mask)
{
  const std::lock_guard<std::mutex> lock(testcases_mutex);

  current_testcase_id += 1;
  log->info("New Testcase %i with EIA %s EEA %s\n",
            current_testcase_id,
            std::bitset<8>(_eia_mask).to_string().c_str(),
            std::bitset<8>(_eea_mask).to_string().c_str());
  testcases[current_testcase_id] = new testcase(log, current_testcase_id, _eia_mask, _eea_mask);
  return current_testcase_id;
}

bool testbench::get_result()
{
  return true;
}

bool testbench::is_finished()
{
  const std::lock_guard<std::mutex> lock(testcases_mutex);

  if (testcases.find(current_testcase_id) == testcases.end()) {
    log->error("is_finished: no testcase %i available.\n", current_testcase_id);
    return false;
  }
  auto tc = testcases[current_testcase_id];
  if (tc != nullptr) {
    return tc->is_finished();
  }
  return false;
}

bool testbench::is_connected()
{
  const std::lock_guard<std::mutex> lock(testcases_mutex);

  if (testcases.find(current_testcase_id) == testcases.end()) {
    log->error("is_connected: no testcase %i available.\n", current_testcase_id);
    return false;
  }
  auto tc = testcases[current_testcase_id];
  if (tc != nullptr) {
    return tc->is_connected();
  }
  return false;
}

std::string testbench::get_current_tc_summary() {
  const std::lock_guard<std::mutex> lock(testcases_mutex);

  if (testcases.find(current_testcase_id) == testcases.end()) {
    log->error("get_summary: no testcase %i available.\n", current_testcase_id);
    return std::string("error");
  }
  auto tc = testcases[current_testcase_id];
  if (tc != nullptr) {
    return tc->get_summary();
  }
  return std::string("error");
}

std::string testbench::get_summary() {
  const std::lock_guard<std::mutex> lock(testcases_mutex);
  log->info("############ SUMMARY ############\n");
  log->info("# Executed testcases: %lu\n", testcases.size());
  log->info("# Following testcases require manual inspection: \n");

  for (auto it = testcases.begin(); it != testcases.end(); it++) {
    if (it->second->is_interesting()) {
      log->info("%s", it->second->get_summary().c_str());
    }
  }
  // TODO iterate through TCs and print some summary, like
  // 123 TCs processed
  // X require manual inspection:
  // TC EIA 0x001...
  // Reason: NULL
  // PCAPs: 
  return std::string("");
}

/* NAS interface */
void testbench::report_nas(){

};

void testbench::report_attach_accept()
{
  const std::lock_guard<std::mutex> lock(testcases_mutex);

  if (testcases.find(current_testcase_id) == testcases.end()) {
    log->error("report_attach_accept: no testcase %i available.\n", current_testcase_id);
    return;
  }
  auto tc = testcases[current_testcase_id];
  if (tc != nullptr) {
    tc->report_attach_accept();
  }
};

void testbench::report_attach_reject(uint8_t _cause)
{
  const std::lock_guard<std::mutex> lock(testcases_mutex);

  if (testcases.find(current_testcase_id) == testcases.end()) {
    log->error("report_attach_reject: no testcase %i available.\n", current_testcase_id);
    return;
  }
  auto tc = testcases[current_testcase_id];
  if (tc != nullptr) {
    tc->report_attach_reject(_cause);
  }
};

void testbench::report_nas_security_mode_command(uint8_t _eia, uint8_t _eea)
{
  const std::lock_guard<std::mutex> lock(testcases_mutex);

  if (testcases.find(current_testcase_id) == testcases.end()) {
    log->error("report_nas_security_mode_command: no testcase %i available.\n", current_testcase_id);
    return;
  }
  auto tc = testcases[current_testcase_id];
  if (tc != nullptr) {
    tc->report_nas_security_mode_command(_eia, _eea);
  }
};

/* RRC interface */
void testbench::report_rrc_key(key_type _type, uint8_t* _k) {
  const std::lock_guard<std::mutex> lock(testcases_mutex);

  if (testcases.find(current_testcase_id) == testcases.end()) {
    log->error("report_rrc_key: no testcase %i available.\n", current_testcase_id);
    return;
  }
  auto tc = testcases[current_testcase_id];
  if (tc != nullptr) {
    tc->report_rrc_key(_type, _k);
  }
};

void testbench::report_rrc_security_mode_command(uint8_t _eia, uint8_t _eea)
{
  const std::lock_guard<std::mutex> lock(testcases_mutex);

  if (testcases.find(current_testcase_id) == testcases.end()) {
    log->error("report_rrc_security_mode_command: no testcase %i available.\n", current_testcase_id);
    return;
  }
  auto tc = testcases[current_testcase_id];
  if (tc != nullptr) {
    tc->report_rrc_security_mode_command(_eia, _eea);
  }
};

testbench::testcase::testcase(srslte::log_filter* _log, uint _id, uint8_t _eia_mask, uint8_t _eea_mask)
{
  log = _log;
  id  = _id;
  for (uint i = 0; i < 8; i++) {
    eia_caps[i] = (_eia_mask & (0b1 << i)) != 0;
    eea_caps[i] = (_eea_mask & (0b1 << i)) != 0;
  }
  bzero(&summary, sizeof(summary_t));

  /*     eia_caps |= eia_caps[0] ? 0b0001 : 0;
      eia_caps |= eia_caps[1] ? 0b0010 : 0;
      eia_caps |= eia_caps[2] ? 0b0100 : 0;
      eia_caps |= eia_caps[3] ? 0b1000 : 0;

      eea_caps |= eea_caps[0] ? 0b0001 : 0;
      eea_caps |= eea_caps[1] ? 0b0010 : 0;
      eea_caps |= eea_caps[2] ? 0b0100 : 0;
      eea_caps |= eea_caps[3] ? 0b1000 : 0; */
  update_summary();
};

bool testbench::testcase::is_interesting()
{
  return summary.is_interesting;
}

bool testbench::testcase::is_finished()
{
  return got_attach_accept || got_attach_reject;
}

bool testbench::testcase::is_connected()
{
  return got_attach_accept && got_nas_security_mode_command && got_rrc_security_mode_command;
}
std::string testbench::testcase::mme_cause_str(uint cause) {
  std::map<uint, std::string> mme_causes;
  mme_causes[0x02] = "MME_EMM_CAUSE_IMSI_UNKNOWN_IN_HSS";
  mme_causes[0x03] = "MME_EMM_CAUSE_ILLEGAL_UE";
  mme_causes[0x05] = "MME_EMM_CAUSE_IMEI_NOT_ACCEPTED";
  mme_causes[0x06] = "MME_EMM_CAUSE_ILLEGAL_ME";
  mme_causes[0x07] = "MME_EMM_CAUSE_EPS_SERVICES_NOT_ALLOWED";
  mme_causes[0x08] = "MME_EMM_CAUSE_EPS_SERVICES_AND_NON_EPS_SERVICES_NOT_ALLOWED";
  mme_causes[0x09] = "MME_EMM_CAUSE_UE_IDENTITY_CANNOT_BE_DERIVED_BY_THE_NETWORK";
  mme_causes[0x0A] = "MME_EMM_CAUSE_IMPLICITLY_DETACHED";
  mme_causes[0x0B] = "MME_EMM_CAUSE_PLMN_NOT_ALLOWED";
  mme_causes[0x0C] = "MME_EMM_CAUSE_TRACKING_AREA_NOT_ALLOWED";
  mme_causes[0x0D] = "MME_EMM_CAUSE_ROAMING_NOT_ALLOWED_IN_THIS_TRACKING_AREA";
  mme_causes[0x0E] = "MME_EMM_CAUSE_EPS_SERVICES_NOT_ALLOWED_IN_THIS_PLMN";
  mme_causes[0x0F] = "MME_EMM_CAUSE_NO_SUITABLE_CELLS_IN_TRACKING_AREA";
  mme_causes[0x10] = "MME_EMM_CAUSE_MSC_TEMPORARILY_NOT_REACHABLE";
  mme_causes[0x11] = "MME_EMM_CAUSE_NETWORK_FAILURE";
  mme_causes[0x12] = "MME_EMM_CAUSE_CS_DOMAIN_NOT_AVAILABLE";
  mme_causes[0x13] = "MME_EMM_CAUSE_ESM_FAILURE";
  mme_causes[0x14] = "MME_EMM_CAUSE_MAC_FAILURE";
  mme_causes[0x15] = "MME_EMM_CAUSE_SYNCH_FAILURE";
  mme_causes[0x16] = "MME_EMM_CAUSE_CONGESTION";
  mme_causes[0x17] = "MME_EMM_CAUSE_UE_SECURITY_CAPABILITIES_MISMATCH";
  mme_causes[0x18] = "MME_EMM_CAUSE_SECURITY_MODE_REJECTED_UNSPECIFIED";
  mme_causes[0x19] = "MME_EMM_CAUSE_NOT_AUTHORIZED_FOR_THIS_CSG";
  mme_causes[0x1A] = "MME_EMM_CAUSE_NON_EPS_AUTHENTICATION_UNACCEPTABLE";
  mme_causes[0x27] = "MME_EMM_CAUSE_CS_SERVICE_TEMPORARILY_NOT_AVAILABLE";
  mme_causes[0x28] = "MME_EMM_CAUSE_NO_EPS_BEARER_CONTEXT_ACTIVATED";
  mme_causes[0x5F] = "MME_EMM_CAUSE_SEMANTICALLY_INCORRECT_MESSAGE";
  mme_causes[0x60] = "MME_EMM_CAUSE_INVALID_MANDATORY_INFORMATION";
  mme_causes[0x61] = "MME_EMM_CAUSE_MESSAGE_TYPE_NON_EXISTENT_OR_NOT_IMPLEMENTED";
  mme_causes[0x62] = "MME_EMM_CAUSE_MESSAGE_TYPE_NOT_COMPATIBLE_WITH_THE_PROTOCOL_STATE";
  mme_causes[0x63] = "MME_EMM_CAUSE_INFORMATION_ELEMENT_NON_EXISTENT_OR_NOT_IMPLEMENTED";
  mme_causes[0x64] = "MME_EMM_CAUSE_CONDITIONAL_IE_ERROR";
  mme_causes[0x65] = "MME_EMM_CAUSE_MESSAGE_NOT_COMPATIBLE_WITH_THE_PROTOCOL_STATE";
  mme_causes[0x6F] = "MME_EMM_CAUSE_PROTOCOL_ERROR_UNSPECIFIED";
  if (mme_causes.find(cause) != mme_causes.end()) {
    return mme_causes.at(cause);
  }
  return "error";
}

void testbench::testcase::set_pcap(std::string _nas_pcap, std::string _mac_pcap) {
  nas_pcap = _nas_pcap;
  mac_pcap = _mac_pcap;

  update_summary();
}

void testbench::testcase::report_nas(){};
void testbench::testcase::report_attach_accept()
{
  log->info("Testcase %u got Attach Accept\n", id);
  got_attach_accept = true;
  update_summary();
};
void testbench::testcase::report_attach_reject(uint8_t _cause)
{
  log->info("Testcase %u got Attach Reject, cause: %s\n", id, mme_cause_str(_cause).c_str());
  got_attach_reject = true;
  update_summary();
};
void testbench::testcase::report_nas_security_mode_command(uint8_t _eia, uint8_t _eea)
{
  log->info("Testcase %u got NAS Security Mode Command. Integrity: %s, Ciphering: %s\n", 
            id,
            srslte::integrity_algorithm_id_text[_eia],
            srslte::ciphering_algorithm_id_text[_eea]);
  nas_eia                       = _eia;
  nas_eea                       = _eea;
  got_nas_security_mode_command = true;
  update_summary();
};
void testbench::testcase::report_rrc_security_mode_command(uint8_t _eia, uint8_t _eea)
{
  log->info("Testcase %u got RRC Security Mode Command. Integrity: %s, Ciphering: %s\n",
            id,
            srslte::integrity_algorithm_id_text[_eia],
            srslte::ciphering_algorithm_id_text[_eea]);
  rrc_eia                       = _eia;
  rrc_eea                       = _eea;
  got_rrc_security_mode_command = true;
  update_summary();
};

void testbench::testcase::report_rrc_key(key_type _type, uint8_t* _k) {
  if (_k == nullptr) {
    return;
  }
  switch (_type) {
    case key_type::k_rrc_enc:
      memcpy(k_rrc_enc, _k, 32);
      log->info_hex(k_rrc_enc, 32, "RRC encryption key - k_rrc_enc");
    break;
    case key_type::k_rrc_int:
      memcpy(k_rrc_int, _k, 32);
      log->info_hex(k_rrc_int, 32, "RRC integrity key - k_rrc_int");
    break;
    case key_type::k_up_enc:
      memcpy(k_up_enc, _k, 32);
      log->info_hex(k_up_enc, 32, "UP encryption key - k_up_enc");
    break;
    default:
      log->error("Error setting key, testcase %i\n", id);
  }
  update_summary();
}

void testbench::testcase::update_summary() {
    summary.insecure_nas_eea_choice = got_nas_security_mode_command && ( nas_eea == 0 || nas_eea > 3 );
    summary.insecure_nas_eia_choice = got_nas_security_mode_command && ( nas_eia == 0 || nas_eia > 3 );
    summary.insecure_rrc_eea_choice = got_rrc_security_mode_command && ( rrc_eea == 0 || rrc_eea > 3 );
    summary.insecure_rrc_eia_choice = got_rrc_security_mode_command && ( rrc_eia == 0 || rrc_eia > 3 );

    summary.spare_values = (nas_eia > 3 || nas_eea > 3 || rrc_eia > 3 || rrc_eea > 3); 
    summary.success =  (got_attach_accept || got_rrc_security_mode_command || got_nas_security_mode_command);

    summary.is_interesting = summary.insecure_nas_eea_choice || summary.insecure_nas_eia_choice || \
                              summary.insecure_rrc_eea_choice || summary.insecure_rrc_eia_choice || \
                              summary.spare_values;
}

// TODO fix this mess
std::string testbench::testcase::get_summary() {
    update_summary();

    std::stringstream ss;
    // if either AES or Snow3G are included we'd expect a secure, successful connection
    bool has_secure_capabilities = ( eia_caps[1] || eia_caps[2] )
                                && ( eea_caps[1] || eea_caps[2] );

    // TODO learn how to work with CLI…
    ss << "UE Security Capabilities: Encryption Integrity" << std::endl;
 // ss << "              NULL   :     x          -    "
    ss << "                 NULL   :     "<< (eea_caps[0]? "x":"-") <<"          "<< (eia_caps[0]? "x":"-") <<"    " << std::endl;
    ss << "                 Snow3G :     "<< (eea_caps[1]? "x":"-") <<"          "<< (eia_caps[1]? "x":"-") <<"    " << std::endl;
    ss << "                 AES    :     "<< (eea_caps[2]? "x":"-") <<"          "<< (eia_caps[2]? "x":"-") <<"    " << std::endl;
    ss << "                 ZUC    :     "<< (eea_caps[3]? "x":"-") <<"          "<< (eia_caps[3]? "x":"-") <<"    " << std::endl;

    ss << "Received Messages: " << std::endl;
    if (got_nas_security_mode_command) {
      ss << "  * NAS Security Mode Command: EEA=" << srslte::ciphering_algorithm_id_text[nas_eea] << ", EIA=" << srslte::integrity_algorithm_id_text[nas_eia] << std::endl;
    }
    if (got_rrc_security_mode_command) {
      ss << "  * RRC Security Mode Command: EEA=" << srslte::ciphering_algorithm_id_text[rrc_eea] << ", EIA=" << srslte::integrity_algorithm_id_text[rrc_eia] << std::endl;
    }
    if (got_attach_accept) {
      ss << "  * Attach Accept" << std::endl;
    }
    if (got_attach_reject) {
      ss << "  * Attach Reject" << std::endl;
    }

    bool any = false;
    ss << "Potential issues: " << std::endl;
    if (summary.success && (summary.insecure_nas_eea_choice || summary.insecure_rrc_eea_choice)) {
      ss << "  * Insecure ciphering EEA detected!" << std::endl;
      any = true;
    }
    if (summary.success && (summary.insecure_nas_eia_choice || summary.insecure_rrc_eia_choice)) {
      ss << "  * Insecure ciphering EIA detected!" << std::endl;
      any = true;
    }
    if (summary.success && summary.spare_values) {
      ss << "  * Insecure ciphering EIA detected!" << std::endl;
      any = true;
    }
    if (!any) {
      ss << " -- no issues -- " << std::endl;
    }
    // TODO reject despite secure config?

    return ss.str();
}

} // namespace srsue
