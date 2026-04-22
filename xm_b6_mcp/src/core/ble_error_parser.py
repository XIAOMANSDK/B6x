"""
BLE Error Code Parser and Knowledge Base
========================================

Parses ble/api/le_err.h and provides:
1. Error code translation (hex → name → description)
2. Troubleshooting guidance for each error
3. AI-powered error analysis

Author: B6x MCP Server Team
Version: 1.0.0
"""

import re
import json
import logging
from pathlib import Path
from dataclasses import dataclass, field, asdict
from typing import Dict, List, Optional

logger = logging.getLogger(__name__)


@dataclass
class BleErrorCode:
    """BLE error code definition."""
    code: int  # Hex error code (e.g., 0x40)
    name: str  # Error name (e.g., GAP_ERR_INVALID_PARAM)
    category: str  # Error category (GAP, ATT, L2C, GATT, SMP, PRF, LL)
    description: str  # Human-readable description
    troubleshooting: List[str] = field(default_factory=list)  # Troubleshooting steps
    common_causes: List[str] = field(default_factory=list)  # Common causes
    # New field: APIs that may return this error
    # Structure: [{"name": "gattc_read_req", "condition": "Reading invalid handle", "probability": "high"}, ...]
    related_apis: List[Dict[str, str]] = field(default_factory=list)


class BleErrorKnowledgeBase:
    """BLE error code knowledge base with troubleshooting guidance."""

    def __init__(self):
        self.errors: Dict[int, BleErrorCode] = {}
        self._build_knowledge_base()

    def _build_knowledge_base(self):
        """Build the complete error code knowledge base."""
        # GAP Errors (0x40-0x4D)
        self._add_gap_errors()

        # ATT Errors (0x01-0x11, 0x80)
        self._add_att_errors()

        # L2C Errors (0x30-0x3F)
        self._add_l2c_errors()

        # GATT Errors (0x50-0x56)
        self._add_gatt_errors()

        # SMP Errors (0x61-0x7C, 0xD0-0xD4)
        self._add_smp_errors()

        # PRF Errors (0x80-0xFF)
        self._add_prf_errors()

        # LL Errors (0x91-0xCE)
        self._add_ll_errors()

        logger.info(f"Built knowledge base with {len(self.errors)} error codes")

    def _add_gap_errors(self):
        """Add GAP (Generic Access Profile) error codes."""
        gap_errors = [
            (0x40, "GAP_ERR_INVALID_PARAM",
             "Invalid parameters set",
             ["Check all parameters in API call", "Verify parameter ranges", "Review API documentation"],
             [  # related_apis
                 {"name": "gapc_connect_req", "condition": "Invalid connection parameters", "probability": "high"},
                 {"name": "gapc_param_update_req", "condition": "Invalid connection interval/latency/timeout", "probability": "high"},
                 {"name": "gapm_start_advertising_req", "condition": "Invalid advertising parameters", "probability": "medium"},
             ]),
            (0x41, "GAP_ERR_PROTOCOL_PROBLEM",
             "Problem with protocol exchange, get unexpected response",
             ["Check peer device compatibility", "Verify BLE version compatibility", "Monitor protocol exchange"],
             [  # related_apis
                 {"name": "gapc_connect_req", "condition": "Protocol incompatibility during connection", "probability": "medium"},
             ]),
            (0x42, "GAP_ERR_NOT_SUPPORTED",
             "Request not supported by software configuration",
             ["Check feature support in SDK", "Verify peripheral capabilities", "Review configuration files"],
             [  # related_apis
                 {"name": "gapm_start_advertising_req", "condition": "Unsupported advertising type", "probability": "medium"},
                 {"name": "gapc_param_update_req", "condition": "Unsupported connection parameters", "probability": "medium"},
             ]),
            (0x43, "GAP_ERR_COMMAND_DISALLOWED",
             "Request not allowed in current state",
             ["Check current connection state", "Verify state machine", "Review command sequence"],
             [  # related_apis
                 {"name": "gapc_connect_req", "condition": "Already connected or wrong state", "probability": "high"},
                 {"name": "gapm_start_advertising_req", "condition": "Advertising already active", "probability": "high"},
                 {"name": "gapm_stop_advertising_req", "condition": "No active advertising to stop", "probability": "medium"},
             ]),
            (0x44, "GAP_ERR_CANCELED",
             "Requested operation canceled",
             ["Check for cancel requests", "Verify user interactions", "Review callback handling"],
             []),
            (0x45, "GAP_ERR_TIMEOUT",
             "Requested operation timeout",
             ["Increase timeout values", "Check peer device responsiveness", "Verify radio conditions"],
             [  # related_apis
                 {"name": "gapc_connect_req", "condition": "Connection attempt timeout", "probability": "high"},
                 {"name": "gapc_param_update_req", "condition": "Parameter update timeout", "probability": "medium"},
                 {"name": "gapc_bond_req", "condition": "Pairing process timeout", "probability": "medium"},
             ]),
            (0x46, "GAP_ERR_DISCONNECTED",
             "Link connection lost during operation",
             ["Check signal strength", "Verify power management", "Monitor connection parameters"],
             [  # related_apis
                 {"name": "gapc_disconnect_req", "condition": "Disconnection initiated", "probability": "medium"},
                 {"name": "gattc_read_req", "condition": "Read operation interrupted by disconnect", "probability": "low"},
                 {"name": "gattc_write_req", "condition": "Write operation interrupted by disconnect", "probability": "low"},
             ]),
            (0x47, "GAP_ERR_NOT_FOUND",
             "Search algorithm finished, but no result found",
             ["Verify device is advertising", "Check scan parameters", "Extend scan duration"],
             [  # related_apis
                 {"name": "gapm_scan_req", "condition": "No device found during scan", "probability": "high"},
                 {"name": "gattc_disc_svc_all_req", "condition": "Service not found", "probability": "medium"},
             ]),
            (0x48, "GAP_ERR_REJECTED",
             "Request rejected by peer device",
             ["Check peer device state", "Verify pairing status", "Review security requirements"],
             [  # related_apis
                 {"name": "gapc_connect_req", "condition": "Connection rejected by peer", "probability": "high"},
                 {"name": "gapc_param_update_req", "condition": "Parameter update rejected by peer", "probability": "medium"},
             ]),
            (0x49, "GAP_ERR_PRIVACY_CFG_PB",
             "Problem with privacy configuration",
             ["Check privacy settings", "Verify address resolution", "Review RPA configuration"],
             [  # related_apis
                 {"name": "gapm_set_dev_config_req", "condition": "Invalid privacy configuration", "probability": "high"},
             ]),
            (0x4A, "GAP_ERR_ADV_DATA_INVALID",
             "Duplicate or invalid advertising data",
             ["Check advertising data format", "Verify data length (max 31 bytes)", "Remove duplicate data"],
             [  # related_apis
                 {"name": "gapm_start_advertising_req", "condition": "Invalid advertising data", "probability": "high"},
                 {"name": "gapm_set_adv_data_req", "condition": "Advertising data exceeds 31 bytes", "probability": "high"},
             ]),
            (0x4B, "GAP_ERR_INSUFF_RESOURCES",
             "Insufficient resources",
             ["Check available memory", "Reduce concurrent operations", "Free unused resources"],
             [  # related_apis
                 {"name": "gapc_connect_req", "condition": "No available connection slots", "probability": "medium"},
             ]),
            (0x4C, "GAP_ERR_UNEXPECTED",
             "Unexpected Error",
             ["Check log for details", "Verify SDK version", "Report bug if persistent"],
             []),
            (0x4D, "GAP_ERR_MISMATCH",
             "Feature mismatch",
             ["Verify feature compatibility", "Check SDK configuration", "Review peer capabilities"],
             [  # related_apis
                 {"name": "gapc_bond_req", "condition": "Pairing feature mismatch", "probability": "medium"},
             ]),
        ]

        for code, name, desc, causes, related_apis in gap_errors:
            self.errors[code] = BleErrorCode(
                code=code, name=name, category="GAP",
                description=desc, common_causes=causes, related_apis=related_apis
            )

    def _add_att_errors(self):
        """Add ATT (Attribute Protocol) error codes."""
        att_errors = [
            (0x01, "ATT_ERR_INVALID_HANDLE",
             "Handle is invalid",
             ["Verify handle value", "Check service discovery", "Ensure attribute exists"],
             [  # related_apis
                 {"name": "gattc_read_req", "condition": "Reading with invalid handle", "probability": "high"},
                 {"name": "gattc_write_req", "condition": "Writing to invalid handle", "probability": "high"},
             ]),
            (0x02, "ATT_ERR_READ_NOT_PERMITTED",
             "Read permission disabled",
             ["Check CCCD configuration", "Verify permissions", "Ensure notifications enabled"],
             [  # related_apis
                 {"name": "gattc_read_req", "condition": "Reading non-readable characteristic", "probability": "high"},
             ]),
            (0x03, "ATT_ERR_WRITE_NOT_PERMITTED",
             "Write permission disabled",
             ["Check attribute permissions", "Verify authentication", "Review CCCD settings"],
             [  # related_apis
                 {"name": "gattc_write_req", "condition": "Writing to non-writable characteristic", "probability": "high"},
             ]),
            (0x04, "ATT_ERR_INVALID_PDU",
             "Incorrect PDU",
             ["Verify MTU size", "Check PDU format", "Review protocol specification"],
             []),
            (0x05, "ATT_ERR_INSUFF_AUTHEN",
             "Authentication privilege not enough",
             ["Authenticate connection", "Check security level", "Verify bonding"],
             [  # related_apis
                 {"name": "gattc_read_req", "condition": "Reading authenticated characteristic without auth", "probability": "high"},
                 {"name": "gattc_write_req", "condition": "Writing authenticated characteristic without auth", "probability": "high"},
             ]),
            (0x06, "ATT_ERR_REQUEST_NOT_SUPPORTED",
             "Request not supported or not understood",
             ["Check feature support", "Verify command type", "Review peer capabilities"],
             []),
            (0x07, "ATT_ERR_INVALID_OFFSET",
             "Incorrect offset value",
             ["Verify offset calculation", "Check data length", "Review read/write logic"],
             [  # related_apis
                 {"name": "gattc_read_long_req", "condition": "Invalid offset in long read", "probability": "medium"},
                 {"name": "gattc_write_long_req", "condition": "Invalid offset in long write", "probability": "medium"},
             ]),
            (0x08, "ATT_ERR_INSUFF_AUTHOR",
             "Authorization privilege not enough",
             ["Check authorization settings", "Verify access rights", "Review security policies"],
             [  # related_apis
                 {"name": "gattc_read_req", "condition": "Reading authorized characteristic without authorization", "probability": "medium"},
                 {"name": "gattc_write_req", "condition": "Writing authorized characteristic without authorization", "probability": "medium"},
             ]),
            (0x09, "ATT_ERR_PREPARE_QUEUE_FULL",
             "Capacity queue for reliable write reached",
             ["Execute prepared writes", "Clear queue", "Reduce batch size"],
             [  # related_apis
                 {"name": "gattc_write_req", "condition": "Prepare write queue full", "probability": "high"},
             ]),
            (0x0A, "ATT_ERR_ATTRIBUTE_NOT_FOUND",
             "Attribute requested not existing",
             ["Verify service UUID", "Check characteristic UUID", "Run service discovery"],
             [  # related_apis
                 {"name": "gattc_read_req", "condition": "Reading non-existent handle", "probability": "high"},
                 {"name": "gattc_write_req", "condition": "Writing to non-existent handle", "probability": "high"},
                 {"name": "gattc_disc_svc_all_req", "condition": "Service discovery no match found", "probability": "medium"},
                 {"name": "gattc_disc_char_all_req", "condition": "Characteristic discovery no match", "probability": "medium"},
             ]),
            (0x0B, "ATT_ERR_ATTRIBUTE_NOT_LONG",
             "Attribute requested not long",
             ["Use regular read", "Check attribute type", "Verify data length"],
             [  # related_apis
                 {"name": "gattc_read_long_req", "condition": "Long read on short attribute", "probability": "high"},
             ]),
            (0x0C, "ATT_ERR_INSUFF_ENC_KEY_SIZE",
             "Encryption size not sufficient",
             ["Increase encryption key size", "Check security requirements", "Verify pairing"],
             [  # related_apis
                 {"name": "gattc_read_req", "condition": "Reading encrypted characteristic with insufficient key", "probability": "medium"},
                 {"name": "gattc_write_req", "condition": "Writing encrypted characteristic with insufficient key", "probability": "medium"},
             ]),
            (0x0D, "ATT_ERR_INVALID_ATTRIBUTE_VAL_LEN",
             "Invalid length of the attribute value",
             ["Check max length", "Verify data format", "Review MTU settings"],
             [  # related_apis
                 {"name": "gattc_write_req", "condition": "Writing value exceeding max length", "probability": "high"},
             ]),
            (0x0E, "ATT_ERR_UNLIKELY_ERR",
             "Operation not fit to condition",
             ["Check operation validity", "Verify state", "Review sequence"],
             []),
            (0x0F, "ATT_ERR_INSUFF_ENC",
             "Attribute requires encryption before operation",
             ["Enable encryption", "Start pairing", "Verify security"],
             [  # related_apis
                 {"name": "gattc_read_req", "condition": "Reading encrypted characteristic without encryption", "probability": "high"},
                 {"name": "gattc_write_req", "condition": "Writing encrypted characteristic without encryption", "probability": "high"},
             ]),
            (0x10, "ATT_ERR_UNSUPP_GRP_TYPE",
             "Attribute grouping not supported",
             ["Check feature support", "Verify group type", "Review capabilities"],
             []),
            (0x11, "ATT_ERR_INSUFF_RESOURCE",
             "Resources not sufficient to complete request",
             ["Free memory", "Reduce concurrent ops", "Check heap size"],
             []),
            (0x80, "ATT_ERR_APP_ERROR",
             "Application error",
             ["Check application code", "Verify error handling", "Review callbacks"],
             [  # related_apis
                 {"name": "gattc_write_req", "condition": "Application rejected write value", "probability": "medium"},
             ]),
        ]

        for code, name, desc, causes, related_apis in att_errors:
            self.errors[code] = BleErrorCode(
                code=code, name=name, category="ATT",
                description=desc, common_causes=causes, related_apis=related_apis
            )

    def _add_l2c_errors(self):
        """Add L2C (Logical Link Control) error codes."""
        l2c_errors = [
            (0x30, "L2C_ERR_CONNECTION_LOST",
             "Message cannot be sent because connection lost (disconnected)",
             ["Check connection status", "Monitor signal strength", "Verify peer device"],
             []),
            (0x31, "L2C_ERR_INVALID_MTU_EXCEED",
             "Invalid PDU length exceed MTU",
             ["Reduce PDU size", "Check MTU configuration", "Segment data"],
             [  # related_apis
                 {"name": "l2cc_send_req", "condition": "PDU size exceeds MTU", "probability": "high"},
             ]),
            (0x32, "L2C_ERR_INVALID_MPS_EXCEED",
             "Invalid PDU length exceed MPS",
             ["Reduce PDU size", "Check MPS settings", "Segment data"],
             [  # related_apis
                 {"name": "l2cc_send_req", "condition": "PDU size exceeds MPS", "probability": "high"},
             ]),
            (0x33, "L2C_ERR_INVALID_CID",
             "Invalid Channel ID",
             ["Verify channel ID", "Check connection", "Review L2CAP setup"],
             [  # related_apis
                 {"name": "l2cc_connect_req", "condition": "Invalid CID requested", "probability": "medium"},
             ]),
            (0x34, "L2C_ERR_INVALID_PDU",
             "Invalid PDU",
             ["Check PDU format", "Verify data", "Review specification"],
             []),
            (0x35, "L2C_ERR_NO_RES_AVAIL",
             "Connection refused - no resources available",
             ["Free resources", "Reduce connections", "Check memory"],
             [  # related_apis
                 {"name": "l2cc_connect_req", "condition": "No L2CAP resources available", "probability": "medium"},
             ]),
            (0x36, "L2C_ERR_INSUFF_AUTHEN",
             "Connection refused - insufficient authentication",
             ["Authenticate device", "Check security level", "Verify pairing"],
             [  # related_apis
                 {"name": "l2cc_connect_req", "condition": "L2CAP channel requires authentication", "probability": "medium"},
             ]),
            (0x37, "L2C_ERR_INSUFF_AUTHOR",
             "Connection refused - insufficient authorization",
             ["Check authorization", "Verify access rights", "Review security policies"],
             []),
            (0x38, "L2C_ERR_INSUFF_ENC_KEY_SIZE",
             "Connection refused - insufficient encryption key size",
             ["Increase key size", "Check encryption settings", "Verify pairing"],
             []),
            (0x39, "L2C_ERR_INSUFF_ENC",
             "Connection Refused - insufficient encryption",
             ["Enable encryption", "Start pairing", "Verify security"],
             []),
            (0x3A, "L2C_ERR_LEPSM_NOT_SUPP",
             "Connection refused - LE_PSM not supported",
             ["Check PSM support", "Verify capabilities", "Use alternative PSM"],
             []),
            (0x3B, "L2C_ERR_INSUFF_CREDIT",
             "No more credit",
             ["Wait for credit", "Reduce send rate", "Check flow control"],
             [  # related_apis
                 {"name": "l2cc_send_req", "condition": "No L2CAP credits available", "probability": "high"},
             ]),
            (0x3C, "L2C_ERR_NOT_UNDERSTOOD",
             "Command not understood by peer device",
             ["Check compatibility", "Verify protocol version", "Review peer capabilities"],
             []),
            (0x3D, "L2C_ERR_CREDIT_ERROR",
             "Credit error, invalid number of credit received",
             ["Verify credit count", "Check flow control", "Reset connection"],
             []),
            (0x3E, "L2C_ERR_CID_ALREADY_ALLOC",
             "Channel identifier already allocated",
             ["Close existing channel", "Use different CID", "Check channel state"],
             [  # related_apis
                 {"name": "l2cc_connect_req", "condition": "CID already in use", "probability": "medium"},
             ]),
            (0x3F, "L2C_ERR_UNKNOWN_PDU",
             "Unknown PDU",
             ["Check PDU type", "Verify compatibility", "Review specification"],
             []),
        ]

        for code, name, desc, causes, related_apis in l2c_errors:
            self.errors[code] = BleErrorCode(
                code=code, name=name, category="L2C",
                description=desc, common_causes=causes, related_apis=related_apis
            )

    def _add_gatt_errors(self):
        """Add GATT (Generic Attribute Profile) error codes."""
        gatt_errors = [
            (0x50, "GATT_ERR_INVALID_ATT_LEN",
             "Problem with ATTC protocol response",
             ["Check response length", "Verify MTU", "Review protocol"],
             []),
            (0x51, "GATT_ERR_INVALID_TYPE_IN_SVC_SEARCH",
             "Error in service search",
             ["Verify service UUID", "Check discovery procedure", "Review service definition"],
             [  # related_apis
                 {"name": "gattc_disc_svc_all_req", "condition": "Invalid service UUID in search", "probability": "medium"},
             ]),
            (0x52, "GATT_ERR_WRITE",
             "Invalid write data",
             ["Check data format", "Verify length", "Review permissions"],
             [  # related_apis
                 {"name": "gattc_write_req", "condition": "Invalid write data format", "probability": "medium"},
             ]),
            (0x53, "GATT_ERR_SIGNED_WRITE",
             "Signed write error",
             ["Verify CSRK", "Check signing", "Review security"],
             [  # related_apis
                 {"name": "gattc_sign_info_ind", "condition": "Signed write signature error", "probability": "medium"},
             ]),
            (0x54, "GATT_ERR_ATTRIBUTE_CLIENT_MISSING",
             "No attribute client defined",
             ["Add CCCD", "Check service structure", "Verify characteristic"],
             []),
            (0x55, "GATT_ERR_ATTRIBUTE_SERVER_MISSING",
             "No attribute server defined",
             ["Check service setup", "Verify characteristic", "Review database"],
             []),
            (0x56, "GATT_ERR_INVALID_PERM",
             "Permission set in service/attribute are invalid",
             ["Check permissions", "Verify security", "Review service definition"],
             [  # related_apis
                 {"name": "gattc_read_req", "condition": "Invalid permission settings", "probability": "low"},
                 {"name": "gattc_write_req", "condition": "Invalid permission settings", "probability": "low"},
             ]),
        ]

        for code, name, desc, causes, related_apis in gatt_errors:
            self.errors[code] = BleErrorCode(
                code=code, name=name, category="GATT",
                description=desc, common_causes=causes, related_apis=related_apis
            )

    def _add_smp_errors(self):
        """Add SMP (Security Manager Protocol) error codes."""
        smp_errors = [
            (0x61, "SMP_ERR_LOC_PASSKEY_ENTRY_FAILED",
             "The user input of pass key failed",
             ["Retry passkey entry", "Check user input", "Verify UI"],
             [  # related_apis
                 {"name": "gapc_bond_req", "condition": "Passkey entry failed during pairing", "probability": "high"},
             ]),
            (0x62, "SMP_ERR_LOC_OOB_NOT_AVAILABLE",
             "The OOB Data is not available",
             ["Provide OOB data", "Check OOB channel", "Use alternative pairing"],
             []),
            (0x63, "SMP_ERR_LOC_AUTH_REQ",
             "Pairing cannot be performed due to IO capabilities",
             ["Check IO capabilities", "Adjust pairing method", "Verify compatibility"],
             [  # related_apis
                 {"name": "gapc_bond_req", "condition": "IO capability mismatch", "probability": "medium"},
             ]),
            (0x64, "SMP_ERR_LOC_CONF_VAL_FAILED",
             "The confirm value does not match",
             ["Retry pairing", "Check calculations", "Verify nonce"],
             []),
            (0x65, "SMP_ERR_LOC_PAIRING_NOT_SUPP",
             "Pairing is not supported by the device",
             ["Enable pairing", "Check features", "Verify SDK config"],
             [  # related_apis
                 {"name": "gapc_bond_req", "condition": "Pairing disabled in config", "probability": "high"},
             ]),
            (0x66, "SMP_ERR_LOC_ENC_KEY_SIZE",
             "Resultant encryption key size is insufficient",
             ["Increase key size", "Check security requirements", "Verify encryption"],
             [  # related_apis
                 {"name": "gapc_bond_req", "condition": "Key size below minimum requirement", "probability": "medium"},
             ]),
            (0x67, "SMP_ERR_LOC_CMD_NOT_SUPPORTED",
             "The SMP command received is not supported",
             ["Check command support", "Verify SMP version", "Review capabilities"],
             []),
            (0x68, "SMP_ERR_LOC_UNSPECIFIED_REASON",
             "Pairing failed due to an unspecified reason",
             ["Check log details", "Verify all parameters", "Retry pairing"],
             []),
            (0x69, "SMP_ERR_LOC_REPEATED_ATTEMPTS",
             "Too little time since last pairing request",
             ["Wait before retry", "Check timeout", "Verify anti-replay protection"],
             [  # related_apis
                 {"name": "gapc_bond_req", "condition": "Repeated pairing attempt too soon", "probability": "high"},
             ]),
            (0x6A, "SMP_ERR_LOC_INVALID_PARAM",
             "Command length is invalid or parameter out of range",
             ["Check parameter ranges", "Verify length", "Review specification"],
             []),
            (0x6B, "SMP_ERR_LOC_DHKEY_CHECK_FAILED",
             "DHKey Check value doesn't match",
             ["Retry pairing", "Check DHKey calculation", "Verify secure connections"],
             [  # related_apis
                 {"name": "gapc_bond_req", "condition": "Secure connections DHKey mismatch", "probability": "medium"},
             ]),
            (0x6C, "SMP_ERR_LOC_NUMERIC_COMPARISON_FAILED",
             "Confirm values in numeric comparison do not match",
             ["Retry comparison", "Check user input", "Verify display"],
             [  # related_apis
                 {"name": "gapc_bond_req", "condition": "Numeric comparison rejected", "probability": "high"},
             ]),
            (0x71, "SMP_ERR_REM_PASSKEY_ENTRY_FAILED",
             "Remote: User input of pass key failed",
             ["Ask remote user to retry", "Check remote UI", "Verify pairing method"],
             []),
            (0x72, "SMP_ERR_REM_OOB_NOT_AVAILABLE",
             "Remote: OOB Data is not available",
             ["Provide OOB data to remote", "Check remote OOB", "Use alternative pairing"],
             []),
            (0x73, "SMP_ERR_REM_AUTH_REQ",
             "Remote: Pairing cannot be performed due to IO capabilities",
             ["Check remote IO capabilities", "Adjust pairing method", "Verify compatibility"],
             []),
            (0x74, "SMP_ERR_REM_CONF_VAL_FAILED",
             "Remote: Confirm value does not match",
             ["Ask remote to retry", "Verify remote calculations", "Check compatibility"],
             []),
            (0x75, "SMP_ERR_REM_PAIRING_NOT_SUPP",
             "Remote: Pairing is not supported",
             ["Check remote features", "Verify pairing enabled", "Use alternative security"],
             []),
            (0x76, "SMP_ERR_REM_ENC_KEY_SIZE",
             "Remote: Encryption key size is insufficient",
             ["Check remote encryption", "Verify key size requirements", "Adjust security"],
             []),
            (0x77, "SMP_ERR_REM_CMD_NOT_SUPPORTED",
             "Remote: SMP command not supported",
             ["Check remote capabilities", "Verify SMP version", "Use alternative command"],
             []),
            (0x78, "SMP_ERR_REM_UNSPECIFIED_REASON",
             "Remote: Pairing failed for unspecified reason",
             ["Check remote device", "Verify compatibility", "Retry pairing"],
             []),
            (0x79, "SMP_ERR_REM_REPEATED_ATTEMPTS",
             "Remote: Too little time since last pairing",
             ["Wait before retry", "Check remote timeout", "Verify anti-replay"],
             []),
            (0x7A, "SMP_ERR_REM_INVALID_PARAM",
             "Remote: Invalid parameter",
             ["Check remote parameters", "Verify compatibility", "Retry with valid params"],
             []),
            (0x7B, "SMP_ERR_REM_DHKEY_CHECK_FAILED",
             "Remote: DHKey Check doesn't match",
             ["Ask remote to retry", "Verify remote secure connections", "Check compatibility"],
             []),
            (0x7C, "SMP_ERR_REM_NUMERIC_COMPARISON_FAILED",
             "Remote: Numeric comparison failed",
             ["Ask remote to retry", "Check remote display", "Verify user input"],
             []),
            (0xD0, "SMP_ERR_ADDR_RESOLV_FAIL",
             "The provided resolvable address has not been resolved",
             ["Check resolving list", "Verify IRK", "Add peer to resolving list"],
             [  # related_apis
                 {"name": "gapm_addr_renew_req", "condition": "Address resolution failed", "probability": "medium"},
             ]),
            (0xD1, "SMP_ERR_SIGN_VERIF_FAIL",
             "The Signature Verification Failed",
             ["Verify CSRK", "Check signature", "Verify data integrity"],
             []),
            (0xD2, "SMP_ERR_ENC_KEY_MISSING",
             "Slave device didn't find the LTK needed for encryption",
             ["Check bonding data", "Verify LTK storage", "Re-pair if needed"],
             [  # related_apis
                 {"name": "gapc_encrypt_req", "condition": "No LTK stored for peer", "probability": "high"},
             ]),
            (0xD3, "SMP_ERR_ENC_NOT_SUPPORTED",
             "Slave device doesn't support encryption",
             ["Check encryption support", "Verify capabilities", "Use alternative security"],
             []),
            (0xD4, "SMP_ERR_ENC_TIMEOUT",
             "Timeout occurred during encryption session",
             ["Check timeout settings", "Verify responsiveness", "Retry encryption"],
             []),
        ]

        for code, name, desc, causes, related_apis in smp_errors:
            self.errors[code] = BleErrorCode(
                code=code, name=name, category="SMP",
                description=desc, common_causes=causes, related_apis=related_apis
            )

    def _add_prf_errors(self):
        """Add Profile (PRF) error codes."""
        prf_errors = [
            (0x80, "PRF_ERR_APP_ERROR",
             "Application Error",
             ["Check application code", "Verify error handling", "Review callbacks"],
             [  # related_apis
                 {"name": "prf_write_req", "condition": "Application rejected write", "probability": "medium"},
             ]),
            (0x81, "PRF_ERR_INVALID_PARAM",
             "Invalid parameter in request",
             ["Check parameters", "Verify ranges", "Review API documentation"],
             [  # related_apis
                 {"name": "prf_read_req", "condition": "Invalid read parameters", "probability": "high"},
                 {"name": "prf_write_req", "condition": "Invalid write parameters", "probability": "high"},
             ]),
            (0x82, "PRF_ERR_INEXISTENT_HDL",
             "Inexistent handle for sending read/write request",
             ["Verify handle", "Check service discovery", "Ensure attribute exists"],
             [  # related_apis
                 {"name": "prf_read_req", "condition": "Reading with non-existent handle", "probability": "high"},
                 {"name": "prf_write_req", "condition": "Writing to non-existent handle", "probability": "high"},
             ]),
            (0x83, "PRF_ERR_STOP_DISC_CHAR_MISSING",
             "Discovery stopped due to missing attribute",
             ["Check service definition", "Verify characteristic", "Review specification"],
             [  # related_apis
                 {"name": "prf_disc_svc_req", "condition": "Required characteristic not found", "probability": "medium"},
             ]),
            (0x84, "PRF_ERR_MULTIPLE_SVC",
             "Too many SVC instances found",
             ["Verify service UUID", "Check for duplicates", "Review service structure"],
             []),
            (0x85, "PRF_ERR_STOP_DISC_WRONG_CHAR_PROP",
             "Discovery stopped due to incorrect properties",
             ["Check characteristic properties", "Verify permissions", "Review definition"],
             []),
            (0x86, "PRF_ERR_MULTIPLE_CHAR",
             "Too many Characteristic instances found",
             ["Verify characteristic UUID", "Check for duplicates", "Review service"],
             []),
            (0x87, "PRF_ERR_NOT_WRITABLE",
             "Attribute write not allowed",
             ["Check permissions", "Verify CCCD", "Review properties"],
             [  # related_apis
                 {"name": "prf_write_req", "condition": "Writing to read-only characteristic", "probability": "high"},
             ]),
            (0x88, "PRF_ERR_NOT_READABLE",
             "Attribute read not allowed",
             ["Check permissions", "Verify properties", "Review CCCD"],
             [  # related_apis
                 {"name": "prf_read_req", "condition": "Reading write-only characteristic", "probability": "high"},
             ]),
            (0x89, "PRF_ERR_REQ_DISALLOWED",
             "Request not allowed",
             ["Check state", "Verify permissions", "Review operation validity"],
             []),
            (0x8A, "PRF_ERR_NTF_DISABLED",
             "Notification Not Enabled",
             ["Enable notifications", "Check CCCD", "Verify client configuration"],
             [  # related_apis
                 {"name": "prf_send_ntf_req", "condition": "Sending notification without CCCD enabled", "probability": "high"},
             ]),
            (0x8B, "PRF_ERR_IND_DISABLED",
             "Indication Not Enabled",
             ["Enable indications", "Check CCCD", "Verify client configuration"],
             [  # related_apis
                 {"name": "prf_send_ind_req", "condition": "Sending indication without CCCD enabled", "probability": "high"},
             ]),
            (0x8C, "PRF_ERR_FEATURE_NOT_SUPPORTED",
             "Feature not supported by profile",
             ["Check profile features", "Verify capabilities", "Review SDK config"],
             []),
            (0x8D, "PRF_ERR_UNEXPECTED_LEN",
             "Read value has unexpected length",
             ["Check expected length", "Verify data format", "Review MTU"],
             [  # related_apis
                 {"name": "prf_read_req", "condition": "Read value length mismatch", "probability": "medium"},
             ]),
            (0x8E, "PRF_ERR_DISCONNECTED",
             "Disconnection occurs",
             ["Check connection", "Monitor signal", "Verify peer device"],
             []),
            (0x8F, "PRF_ERR_PROC_TIMEOUT",
             "Procedure Timeout",
             ["Increase timeout", "Check peer responsiveness", "Verify radio"],
             []),
            (0xFD, "PRF_CCCD_IMPR_CONFIGURED",
             "Client characteristic configuration improperly configured",
             ["Check CCCD value", "Verify configuration", "Review client setup"],
             []),
            (0xFE, "PRF_PROC_IN_PROGRESS",
             "Procedure already in progress",
             ["Wait for completion", "Check state", "Avoid concurrent operations"],
             []),
            (0xFF, "PRF_OUT_OF_RANGE",
             "Out of Range",
             ["Check parameter ranges", "Verify values", "Review limits"],
             []),
        ]

        for code, name, desc, causes, related_apis in prf_errors:
            self.errors[code] = BleErrorCode(
                code=code, name=name, category="PRF",
                description=desc, common_causes=causes, related_apis=related_apis
            )

    def _add_ll_errors(self):
        """Add LL (Link Layer) error codes."""
        ll_errors = [
            (0x91, "LL_ERR_UNKNOWN_HCI_COMMAND",
             "Unknown HCI Command",
             ["Check command support", "Verify controller", "Review specification"],
             []),
            (0x92, "LL_ERR_UNKNOWN_CONNECTION_ID",
             "Unknown Connection Identifier",
             ["Verify connection handle", "Check connection status", "Review HCI commands"],
             [  # related_apis
                 {"name": "gapc_disconnect_req", "condition": "Invalid connection handle", "probability": "high"},
             ]),
            (0x93, "LL_ERR_HARDWARE_FAILURE",
             "Hardware Failure",
             ["Reset hardware", "Check chip status", "Contact vendor support"],
             []),
            (0x94, "LL_ERR_PAGE_TIMEOUT",
             "BT Page Timeout",
             ["Increase timeout", "Check page settings", "Verify peer device"],
             []),
            (0x95, "LL_ERR_AUTH_FAILURE",
             "Authentication failure",
             ["Verify authentication", "Check security keys", "Retry pairing"],
             [  # related_apis
                 {"name": "gapc_bond_req", "condition": "Authentication failed during pairing", "probability": "high"},
             ]),
            (0x96, "LL_ERR_PIN_MISSING",
             "Pin code missing",
             ["Provide PIN", "Check pairing method", "Verify security"],
             []),
            (0x97, "LL_ERR_MEMORY_CAPA_EXCEED",
             "Memory capacity exceed",
             ["Free memory", "Reduce connections", "Check heap"],
             []),
            (0x98, "LL_ERR_CON_TIMEOUT",
             "Connection Timeout",
             ["Check supervision timeout", "Verify connection parameters", "Monitor signal"],
             [  # related_apis
                 {"name": "gapc_connect_req", "condition": "Connection timeout", "probability": "high"},
             ]),
            (0x99, "LL_ERR_CON_LIMIT_EXCEED",
             "Connection limit Exceed",
             ["Close unused connections", "Check max connections", "Review controller limits"],
             [  # related_apis
                 {"name": "gapc_connect_req", "condition": "Max connections reached", "probability": "high"},
             ]),
            (0x9A, "LL_ERR_SYNC_CON_LIMIT_DEV_EXCEED",
             "Synchronous Connection limit exceed",
             ["Close SCO connections", "Check limits", "Review audio setup"],
             []),
            (0x9B, "LL_ERR_ACL_CON_EXISTS",
             "ACL Connection exists",
             ["Check existing connection", "Use existing handle", "Verify state"],
             []),
            (0x9C, "LL_ERR_COMMAND_DISALLOWED",
             "Command Disallowed",
             ["Check state", "Verify command validity", "Review sequence"],
             [  # related_apis
                 {"name": "gapm_start_advertising_req", "condition": "Command not allowed in current state", "probability": "medium"},
             ]),
            (0x9D, "LL_ERR_CONN_REJ_LIMITED_RESOURCES",
             "Connection rejected due to limited resources",
             ["Free resources", "Reduce connections", "Check peer device"],
             []),
            (0x9E, "LL_ERR_CONN_REJ_SECURITY_REASONS",
             "Connection rejected due to security reason",
             ["Check security requirements", "Verify authentication", "Review pairing"],
             []),
            (0x9F, "LL_ERR_CONN_REJ_UNACCEPTABLE_BDADDR",
             "Connection rejected due to unacceptable BD Addr",
             ["Check address", "Verify filter list", "Review device white/blacklist"],
             []),
            (0xA0, "LL_ERR_CONN_ACCEPT_TIMEOUT_EXCEED",
             "Connection rejected due to Accept connection timeout",
             ["Increase timeout", "Check responsiveness", "Verify peer device"],
             []),
            (0xA1, "LL_ERR_UNSUPPORTED",
             "Not Supported",
             ["Check feature support", "Verify capabilities", "Review specification"],
             []),
            (0xA2, "LL_ERR_INVALID_HCI_PARAM",
             "Invalid parameters",
             ["Check parameters", "Verify ranges", "Review documentation"],
             []),
            (0xA3, "LL_ERR_REMOTE_USER_TERM_CON",
             "Remote user terminate connection",
             ["Check peer device", "Verify user action", "Review remote logs"],
             []),
            (0xA4, "LL_ERR_REMOTE_DEV_TERM_LOW_RESOURCES",
             "Remote device terminate connection due to low resources",
             ["Check peer device status", "Reduce data rate", "Verify memory"],
             []),
            (0xA5, "LL_ERR_REMOTE_DEV_POWER_OFF",
             "Remote device terminate connection due to power off",
             ["Check peer device", "Verify power state", "Review remote status"],
             []),
            (0xA6, "LL_ERR_CON_TERM_BY_LOCAL_HOST",
             "Connection terminated by local host",
             ["Check local commands", "Verify termination reason", "Review application code"],
             []),
            (0xA7, "LL_ERR_REPEATED_ATTEMPTS",
             "Repeated attempts",
             ["Wait before retry", "Check anti-replay", "Verify timer"],
             []),
            (0xA8, "LL_ERR_PAIRING_NOT_ALLOWED",
             "Pairing not Allowed",
             ["Allow pairing", "Check security settings", "Verify configuration"],
             [  # related_apis
                 {"name": "gapc_bond_req", "condition": "Pairing not allowed by config", "probability": "high"},
             ]),
            (0xA9, "LL_ERR_UNKNOWN_LMP_PDU",
             "Unknown PDU Error",
             ["Check PDU type", "Verify compatibility", "Review specification"],
             []),
            (0xAA, "LL_ERR_UNSUPPORTED_REMOTE_FEATURE",
             "Unsupported remote feature",
             ["Check feature support", "Verify capabilities", "Review compatibility"],
             []),
            (0xAB, "LL_ERR_SCO_OFFSET_REJECTED",
             "Sco Offset rejected",
             ["Check SCO parameters", "Verify offset", "Review audio setup"],
             []),
            (0xAC, "LL_ERR_SCO_INTERVAL_REJECTED",
             "SCO Interval Rejected",
             ["Check SCO parameters", "Verify interval", "Review audio setup"],
             []),
            (0xAD, "LL_ERR_SCO_AIR_MODE_REJECTED",
             "SCO air mode Rejected",
             ["Check air mode", "Verify settings", "Review audio config"],
             []),
            (0xAE, "LL_ERR_INVALID_LMP_PARAM",
             "Invalid LMP parameters",
             ["Check parameters", "Verify ranges", "Review specification"],
             []),
            (0xAF, "LL_ERR_UNSPECIFIED_ERROR",
             "Unspecified error",
             ["Check logs", "Verify all parameters", "Report bug if persistent"],
             []),
            (0xB0, "LL_ERR_UNSUPPORTED_LMP_PARAM_VALUE",
             "Unsupported LMP Parameter value",
             ["Check parameter", "Verify supported values", "Review capabilities"],
             []),
            (0xB1, "LL_ERR_ROLE_CHANGE_NOT_ALLOWED",
             "Role Change Not allowed",
             ["Check role switch settings", "Verify current role", "Review configuration"],
             []),
            (0xB2, "LL_ERR_LMP_RSP_TIMEOUT",
             "LMP Response timeout",
             ["Increase timeout", "Check peer responsiveness", "Verify radio"],
             []),
            (0xB3, "LL_ERR_LMP_COLLISION",
             "LMP Collision",
             ["Retry operation", "Check timing", "Verify state"],
             []),
            (0xB4, "LL_ERR_LMP_PDU_NOT_ALLOWED",
             "LMP Pdu not allowed",
             ["Check PDU type", "Verify state", "Review sequence"],
             []),
            (0xB5, "LL_ERR_ENC_MODE_NOT_ACCEPT",
             "Encryption mode not accepted",
             ["Check encryption mode", "Verify capabilities", "Review security"],
             []),
            (0xB6, "LL_ERR_LINK_KEY_CANT_CHANGE",
             "Link Key Cannot be changed",
             ["Check key type", "Verify bonding", "Review security"],
             []),
            (0xB7, "LL_ERR_QOS_NOT_SUPPORTED",
             "Quality of Service not supported",
             ["Check QOS support", "Verify capabilities", "Adjust parameters"],
             []),
            (0xB8, "LL_ERR_INSTANT_PASSED",
             "Error, instant passed",
             ["Check timing", "Verify instant value", "Review clock"],
             []),
            (0xB9, "LL_ERR_PAIRING_WITH_UNIT_KEY_NOT_SUP",
             "Pairing with unit key not supported",
             ["Check pairing method", "Use alternative key", "Verify security"],
             []),
            (0xBA, "LL_ERR_DIFF_TRANSACTION_COLLISION",
             "Transaction collision",
             ["Retry operation", "Check timing", "Verify state"],
             []),
            (0xBC, "LL_ERR_QOS_UNACCEPTABLE_PARAM",
             "Unacceptable parameters",
             ["Check QOS parameters", "Verify ranges", "Adjust values"],
             []),
            (0xBD, "LL_ERR_QOS_REJECTED",
             "Quality of Service rejected",
             ["Check QOS settings", "Verify capabilities", "Adjust parameters"],
             []),
            (0xBE, "LL_ERR_CHANNEL_CLASS_NOT_SUP",
             "Channel class not supported",
             ["Check channel map", "Verify capabilities", "Review RF settings"],
             []),
            (0xBF, "LL_ERR_INSUFFICIENT_SECURITY",
             "Insufficient security",
             ["Increase security level", "Verify encryption", "Check authentication"],
             [  # related_apis
                 {"name": "gapc_bond_req", "condition": "Security level insufficient", "probability": "medium"},
             ]),
            (0xC0, "LL_ERR_PARAM_OUT_OF_MAND_RANGE",
             "Parameters out of mandatory range",
             ["Check parameter ranges", "Verify values", "Adjust to valid range"],
             []),
            (0xC2, "LL_ERR_ROLE_SWITCH_PEND",
             "Role switch pending",
             ["Wait for completion", "Check role switch state", "Verify timing"],
             []),
            (0xC4, "LL_ERR_RESERVED_SLOT_VIOLATION",
             "Reserved slot violation",
             ["Check scheduling", "Verify slot allocation", "Review timing"],
             []),
            (0xC5, "LL_ERR_ROLE_SWITCH_FAIL",
             "Role Switch fail",
             ["Retry role switch", "Check capabilities", "Verify configuration"],
             []),
            (0xC6, "LL_ERR_EIR_TOO_LARGE",
             "Error, EIR too large",
             ["Reduce EIR size", "Check data length", "Verify limits"],
             [  # related_apis
                 {"name": "gapm_start_advertising_req", "condition": "EIR data too large", "probability": "medium"},
             ]),
            (0xC7, "LL_ERR_SP_NOT_SUPPORTED_HOST",
             "Simple pairing not supported by host",
             ["Enable secure connections", "Check host support", "Verify features"],
             []),
            (0xC8, "LL_ERR_HOST_BUSY_PAIRING",
             "Host pairing is busy",
             ["Wait for completion", "Check pairing state", "Avoid concurrent pairing"],
             [  # related_apis
                 {"name": "gapc_bond_req", "condition": "Another pairing in progress", "probability": "high"},
             ]),
            (0xCA, "LL_ERR_CONTROLLER_BUSY",
             "Controller is busy",
             ["Wait for completion", "Check controller state", "Retry later"],
             []),
            (0xCB, "LL_ERR_UNACCEPTABLE_CONN_INT",
             "Unacceptable connection initialization",
             ["Check connection parameters", "Verify values", "Adjust intervals"],
             []),
            (0xCC, "LL_ERR_DIRECT_ADV_TO",
             "Direct Advertising Timeout",
             ["Increase timeout", "Check advertising", "Verify peer device"],
             [  # related_apis
                 {"name": "gapm_start_advertising_req", "condition": "Direct advertising timeout", "probability": "high"},
             ]),
            (0xCD, "LL_ERR_TERMINATED_MIC_FAILURE",
             "Connection terminated due to MIC failure",
             ["Check encryption", "Verify keys", "Re-pair device"],
             []),
            (0xCE, "LL_ERR_CONN_FAILED_TO_BE_EST",
             "Connection failed to be established",
             ["Check parameters", "Verify peer device", "Retry connection"],
             [  # related_apis
                 {"name": "gapc_connect_req", "condition": "Connection establishment failed", "probability": "high"},
             ]),
        ]

        for code, name, desc, causes, related_apis in ll_errors:
            self.errors[code] = BleErrorCode(
                code=code, name=name, category="LL",
                description=desc, common_causes=causes, related_apis=related_apis
            )

    def lookup_error(self, error_code: int) -> Optional[BleErrorCode]:
        """Look up an error code by its value."""
        return self.errors.get(error_code)

    def lookup_error_by_name(self, name: str) -> Optional[BleErrorCode]:
        """Look up an error code by its name."""
        for error in self.errors.values():
            if error.name == name:
                return error
        return None

    def search_errors(self, keyword: str) -> List[BleErrorCode]:
        """Search for errors by keyword in name or description."""
        keyword_lower = keyword.lower()
        return [
            error for error in self.errors.values()
            if keyword_lower in error.name.lower() or
               keyword_lower in error.description.lower()
        ]

    def get_errors_by_category(self, category: str) -> List[BleErrorCode]:
        """Get all errors in a category."""
        return [
            error for error in self.errors.values()
            if error.category == category
        ]

    def export_to_json(self, output_path: str):
        """Export knowledge base to JSON file."""
        errors_dict = {
            str(code): asdict(error)
            for code, error in self.errors.items()
        }

        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(errors_dict, f, indent=2, ensure_ascii=False)

        logger.info(f"Exported {len(self.errors)} error codes to {output_path}")


# ============================================================================
# AI Error Analysis Assistant
# ============================================================================

class BleErrorAssistant:
    """AI assistant for BLE error code analysis and troubleshooting."""

    def __init__(self, knowledge_base: BleErrorKnowledgeBase = None):
        self.kb = knowledge_base or BleErrorKnowledgeBase()

    def analyze_error(self, error_code: int) -> str:
        """
        Analyze an error code and provide troubleshooting guidance.

        Args:
            error_code: Error code (e.g., 0x40, 64)

        Returns:
            Formatted analysis with troubleshooting steps.
        """
        error = self.kb.lookup_error(error_code)

        if not error:
            return self._unknown_error_response(error_code)

        response = []
        response.append(f"=" * 70)
        response.append(f"BLE Error Analysis: 0x{error.code:02X}")
        response.append("=" * 70)
        response.append(f"\nCategory:    {error.category}")
        response.append(f"Name:        {error.name}")
        response.append(f"Description: {error.description}")

        if error.common_causes:
            response.append(f"\nCommon Causes:")
            for i, cause in enumerate(error.common_causes, 1):
                response.append(f"  {i}. {cause}")

        if error.troubleshooting:
            response.append(f"\nTroubleshooting Steps:")
            for i, step in enumerate(error.troubleshooting, 1):
                response.append(f"  {i}. {step}")
        elif error.common_causes:
            # Use common causes as troubleshooting if no specific steps
            response.append(f"\nRecommended Actions:")
            for i, action in enumerate(error.common_causes[:3], 1):
                response.append(f"  {i}. {action}")

        response.append("\n" + "=" * 70)

        return "\n".join(response)

    def _unknown_error_response(self, error_code: int) -> str:
        """Generate response for unknown error codes."""
        return f"""{"=" * 70}
BLE Error Analysis: 0x{error_code:02X}
{"=" * 70}

Status: UNKNOWN ERROR CODE

This error code is not in the knowledge base.
Possible reasons:
  1. Custom application error (check your code)
  2. New error code in updated SDK
  3. Invalid error code

Recommended Actions:
  1. Search SDK documentation for 0x{error_code:02X}
  2. Check le_err.h for new error definitions
  3. Review application code for custom errors

{"=" * 70}"""

    def parse_log_line(self, log_line: str) -> list:
        """
        Parse a log line and extract error codes.

        Supports formats:
        - "Error: 0x40"
        - "GAP_ERR_INVALID_PARAM (0x40)"
        - "[ERROR] Code=64"
        """
        error_codes = []

        # Match hex patterns: 0x40, 0X40, 0x40, etc.
        hex_pattern = r'0[xX]?([0-9A-Fa-f]{2})'
        matches = re.findall(hex_pattern, log_line)

        for match in matches:
            code = int(match, 16)
            # Filter to valid BLE error ranges
            if 0x01 <= code <= 0xFF or 0x91 <= code <= 0xCE:
                error_codes.append(code)

        return error_codes

    def analyze_log(self, log_text: str) -> str:
        """
        Analyze a log text and provide error analysis for all found error codes.

        Args:
            log_text: Log text containing error codes

        Returns:
            Formatted analysis for all found errors
        """
        error_codes = set()

        # Parse each line
        for line in log_text.split('\n'):
            codes = self.parse_log_line(line)
            error_codes.update(codes)

        if not error_codes:
            return "No BLE error codes found in log."

        # Analyze each error
        analyses = []
        for code in sorted(error_codes):
            analysis = self.analyze_error(code)
            analyses.append(analysis)

        return "\n\n".join(analyses)


# ============================================================================
# CLI Interface
# ============================================================================

def main():
    """CLI interface for BLE error code analysis."""
    import argparse

    parser = argparse.ArgumentParser(description="BLE Error Code Analyzer")
    parser.add_argument("error", nargs="?", help="Error code (hex or decimal, e.g., 0x40 or 64)")
    parser.add_argument("-n", "--name", help="Search by error name")
    parser.add_argument("-s", "--search", help="Search by keyword")
    parser.add_argument("-c", "--category", help="List errors by category")
    parser.add_argument("--export", help="Export knowledge base to JSON")
    parser.add_argument("-l", "--log", help="Analyze log file")

    args = parser.parse_args()

    kb = BleErrorKnowledgeBase()
    assistant = BleErrorAssistant(kb)

    # Export knowledge base
    if args.export:
        kb.export_to_json(args.export)
        return 0

    # List by category
    if args.category:
        errors = kb.get_errors_by_category(args.category.upper())
        print(f"\n{args.category.upper()} Error Codes ({len(errors)} total):")
        for error in errors:
            print(f"  0x{error.code:02X}: {error.name}")
        return 0

    # Search by keyword
    if args.search:
        errors = kb.search_errors(args.search)
        print(f"\nSearch Results for '{args.search}' ({len(errors)} found):")
        for error in errors:
            print(f"  0x{error.code:02X}: {error.name} - {error.description}")
        return 0

    # Search by name
    if args.name:
        error = kb.lookup_error_by_name(args.name)
        if error:
            print(assistant.analyze_error(error.code))
        else:
            print(f"Error '{args.name}' not found in knowledge base")
        return 0

    # Analyze log file
    if args.log:
        with open(args.log, 'r', encoding='utf-8') as f:
            log_text = f.read()
        print(assistant.analyze_log(log_text))
        return 0

    # Analyze specific error code
    if args.error:
        # Parse error code (hex or decimal)
        if args.error.startswith('0x') or args.error.startswith('0X'):
            code = int(args.error, 16)
        else:
            code = int(args.error)

        print(assistant.analyze_error(code))
        return 0

    # Show help if no arguments
    parser.print_help()
    return 0


if __name__ == "__main__":
    import sys
    sys.exit(main())
