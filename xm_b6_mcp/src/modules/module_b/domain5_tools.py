"""
Domain 5 Tools - Production & Toolchain
========================================

MCP tools for Domain 5 (Production & Toolchain):
- Mass production testing
- Programming tools (JFlash, ISP)
- Certification requirements (FCC, CE, SRRC)
- Toolchain setup and configuration

Author: B6x MCP Server Team
Version: 0.1.0
"""

import logging
from pathlib import Path
from typing import Dict, List, Optional, Any

logger = logging.getLogger(__name__)


# ============================================================================
# Domain 5 Tool Implementations
# ============================================================================

def get_production_guide(topic: str) -> Dict[str, Any]:
    """
    Query production-related guides.

    Topics:
    - "isp": ISP programming tool usage
    - "production": Mass production workflow
    - "testing": Production testing procedures
    - "packaging": Product packaging guidelines
    - "calibration": Device calibration procedures

    Args:
        topic: Production topic to query

    Returns:
        Dictionary with guide information
    """
    # Topic to document mapping (based on SDK documentation)
    topic_documents = {
        'isp': {
            'title': 'ISP Tool Production Guide',
            'source_document': 'MP_Spec/ISP Tool/ISP Tool量产说明文档.docx',
            'sections': [
                'ISP Tool Overview',
                'Hardware Connection',
                'Software Setup',
                'Production Programming Steps',
                'Troubleshooting'
            ],
            'key_steps': [
                '1. Connect ISP dongle to target device',
                '2. Install ISP Tool software',
                '3. Load firmware (.bin file)',
                '4. Configure programming parameters',
                '5. Execute programming',
                '6. Verify programming success'
            ],
            'notes': 'Ensure proper power supply and stable USB connection'
        },
        'production': {
            'title': 'Mass Production Guide',
            'source_document': 'MP_Spec/ISP Tool/ISP Tool量产说明文档.docx',
            'sections': [
                'Production Workflow',
                'Testing Procedures',
                'Quality Control',
                'Packaging Guidelines'
            ],
            'key_steps': [
                '1. PCB Assembly Inspection',
                '2. Firmware Programming',
                '3. Functional Testing',
                '4. RF Testing (if applicable)',
                '5. Final QA Check',
                '6. Packaging'
            ],
            'notes': 'Follow ESD safety procedures during production'
        },
        'testing': {
            'title': 'Production Testing Procedures',
            'source_document': 'MP_Spec/ISP Tool/ISP Tool量产说明文档.docx',
            'sections': [
                'Test Equipment Setup',
                'Functional Tests',
                'RF Parameter Tests',
                'Pass/Fail Criteria'
            ],
            'test_items': [
                'Power consumption test',
                'GPIO functionality test',
                'Communication test (UART/SPI/I2C)',
                'BLE RF test (TX power, sensitivity)',
                'Reset test'
            ],
            'notes': 'Record all test results for traceability'
        },
        'packaging': {
            'title': 'Product Packaging Guidelines',
            'source_document': 'MP_Spec/ISP Tool/ISP Tool量产说明文档.docx',
            'sections': [
                'ESD Protection',
                'Labeling Requirements',
                'Accessory Checklist',
                'Packaging Materials'
            ],
            'guidelines': [
                'Use ESD-safe packaging materials',
                'Include product label with model/version/SN',
                'Include user manual and accessories',
                'Seal packaging with tamper-evident seal'
            ],
            'notes': 'Follow regional packaging regulations'
        },
        'calibration': {
            'title': 'Device Calibration Procedures',
            'source_document': 'MP_Spec/ISP Tool/ISP Tool量产说明文档.docx',
            'sections': [
                'RC Oscillator Calibration',
                'RF Power Calibration',
                'DCDC Calibration',
                'Calibration Data Storage'
            ],
            'key_steps': [
                '1. Enter calibration mode',
                '2. Measure RC oscillator frequency',
                '3. Calculate and store trim value',
                '4. Measure RF output power',
                '5. Adjust PA bias if needed',
                '6. Store calibration data in flash'
            ],
            'notes': 'Calibration data must be stored in designated flash area'
        }
    }

    # Normalize topic
    topic_key = topic.lower().strip()

    if topic_key in topic_documents:
        guide = topic_documents[topic_key]
        return {
            'status': 'success',
            'topic': topic,
            'guide': guide
        }
    else:
        return {
            'status': 'not_found',
            'topic': topic,
            'available_topics': list(topic_documents.keys()),
            'message': f'Unknown topic: {topic}. Available topics: {list(topic_documents.keys())}'
        }


def get_jflash_config(chip_model: str) -> Dict[str, Any]:
    """
    Get JFlash configuration for specified chip model.

    Supported chip models:
    - "QFN32": B6x QFN32 package
    - "QFN48": B6x QFN48 package
    - "QFN64": B6x QFN64 package

    Args:
        chip_model: Chip model/package type

    Returns:
        Dictionary with JFlash configuration steps
    """
    # JFlash configurations based on SDK documentation
    jflash_configs = {
        'qfn32': {
            'chip_name': 'B6x QFN32',
            'flash_size': '512KB',
            'ram_size': '64KB',
            'flash_base': '0x08000000',
            'flash_end': '0x0807FFFF',
            'jtag_chain': {
                'device_position': 0,
                'ir_length': 4,
                'idcode': '0xXXXXXXX'  # Actual IDCODE from SVD
            },
            'flash_segments': [
                {
                    'name': 'FLASH',
                    'start_address': '0x08000000',
                    'size': '0x80000 (512KB)',
                    'algorithm': 'B6x_512.FLM'  # Flash algorithm file
                }
            ],
            'setup_steps': [
                '1. Launch JFlash',
                '2. Create new project',
                '3. Select device: B6x QFN32',
                '4. Configure JTAG/SWD interface',
                '5. Set flash parameters',
                '6. Load flash algorithm'
            ],
            'programming_steps': [
                '1. Connect J-Link to target',
                '2. Erase chip',
                '3. Load .hex or .bin file',
                '4. Program flash',
                '5. Verify programming',
                '6. Reset and run'
            ],
            'source_document': 'SW_Spec/JFlash配置说明.docx'
        },
        'qfn48': {
            'chip_name': 'B6x QFN48',
            'flash_size': '512KB',
            'ram_size': '64KB',
            'flash_base': '0x08000000',
            'flash_end': '0x0807FFFF',
            'jtag_chain': {
                'device_position': 0,
                'ir_length': 4,
                'idcode': '0xXXXXXXX'
            },
            'flash_segments': [
                {
                    'name': 'FLASH',
                    'start_address': '0x08000000',
                    'size': '0x80000 (512KB)',
                    'algorithm': 'B6x_512.FLM'
                }
            ],
            'setup_steps': [
                '1. Launch JFlash',
                '2. Create new project',
                '3. Select device: B6x QFN48',
                '4. Configure JTAG/SWD interface',
                '5. Set flash parameters',
                '6. Load flash algorithm'
            ],
            'programming_steps': [
                '1. Connect J-Link to target',
                '2. Erase chip',
                '3. Load .hex or .bin file',
                '4. Program flash',
                '5. Verify programming',
                '6. Reset and run'
            ],
            'source_document': 'SW_Spec/JFlash配置说明.docx'
        },
        'qfn64': {
            'chip_name': 'B6x QFN64',
            'flash_size': '512KB',
            'ram_size': '64KB',
            'flash_base': '0x08000000',
            'flash_end': '0x0807FFFF',
            'jtag_chain': {
                'device_position': 0,
                'ir_length': 4,
                'idcode': '0xXXXXXXX'
            },
            'flash_segments': [
                {
                    'name': 'FLASH',
                    'start_address': '0x08000000',
                    'size': '0x80000 (512KB)',
                    'algorithm': 'B6x_512.FLM'
                }
            ],
            'setup_steps': [
                '1. Launch JFlash',
                '2. Create new project',
                '3. Select device: B6x QFN64',
                '4. Configure JTAG/SWD interface',
                '5. Set flash parameters',
                '6. Load flash algorithm'
            ],
            'programming_steps': [
                '1. Connect J-Link to target',
                '2. Erase chip',
                '3. Load .hex or .bin file',
                '4. Program flash',
                '5. Verify programming',
                '6. Reset and run'
            ],
            'source_document': 'SW_Spec/JFlash配置说明.docx'
        }
    }

    # Normalize chip model
    model_key = chip_model.lower().replace('-', '').replace('_', '')

    if model_key in jflash_configs:
        return {
            'status': 'success',
            'chip_model': chip_model,
            'config': jflash_configs[model_key]
        }
    else:
        return {
            'status': 'not_found',
            'chip_model': chip_model,
            'available_models': list(jflash_configs.keys()),
            'message': f'Unknown chip model: {chip_model}. Available: {list(jflash_configs.keys())}'
        }


def get_isp_tool_usage(operation: str) -> Dict[str, Any]:
    """
    Query ISP tool usage for specific operations.

    Operations:
    - "program": Firmware programming
    - "erase": Chip erase
    - "read": Read flash/OTP
    - "config": Tool configuration
    - "verify": Programming verification

    Args:
        operation: ISP operation to query

    Returns:
        Dictionary with operation steps
    """
    # ISP tool operations based on SDK documentation
    isp_operations = {
        'program': {
            'operation': 'Firmware Programming',
            'prerequisites': [
                'ISP dongle connected',
                'Target device powered',
                'Firmware file (.bin) ready',
                'ISP Tool software installed'
            ],
            'steps': [
                '1. Launch ISP Tool',
                '2. Select target device',
                '3. Connect to target (press reset if needed)',
                '4. Browse and select firmware file',
                '5. Click "Program"',
                '6. Wait for programming completion',
                '7. Verify success status'
            ],
            'parameters': {
                'baudrate': '115200 (default)',
                'start_address': '0x08000000',
                'file_format': '.bin',
                'verify_after_program': True
            },
            'troubleshooting': [
                'If connection fails: Check TX/RX wiring',
                'If programming fails: Check power supply',
                'If verify fails: Re-program firmware'
            ]
        },
        'erase': {
            'operation': 'Chip Erase',
            'steps': [
                '1. Connect to target device',
                '2. Select "Erase" function',
                '3. Choose erase scope (Full/Selected)',
                '4. Confirm erase operation',
                '5. Wait for erase completion',
                '6. Verify chip is blank (all 0xFF)'
            ],
            'warning': 'Chip erase is irreversible! Ensure backup of important data.',
            'erase_time': 'Approximately 10-30 seconds depending on chip size'
        },
        'read': {
            'operation': 'Read Flash/OTP',
            'steps': [
                '1. Connect to target device',
                '2. Select "Read" function',
                '3. Specify read address and length',
                '4. Choose output file format',
                '5. Execute read operation',
                '6. Save read data to file'
            ],
            'readable_areas': [
                'Flash memory (0x08000000 - 0x0807FFFF)',
                'OTP (One-Time Programmable) area',
                'Configuration bytes'
            ],
            'restriction': 'Some protected areas cannot be read'
        },
        'config': {
            'operation': 'Tool Configuration',
            'configurable_items': [
                'Communication port (COM number)',
                'Baudrate',
                'Default file path',
                'Auto-program settings',
                'Log options'
            ],
            'steps': [
                '1. Open Settings menu',
                '2. Select configurable item',
                '3. Modify value',
                '4. Apply and save settings'
            ]
        },
        'verify': {
            'operation': 'Programming Verification',
            'steps': [
                '1. After programming, select "Verify"',
                '2. Specify original firmware file',
                '3. Execute verification',
                '4. Review verification report'
            ],
            'verification_methods': [
                'CRC32 check',
                'Byte-by-byte comparison',
                'Checksum verification'
            ]
        }
    }

    # Normalize operation
    op_key = operation.lower().strip()

    if op_key in isp_operations:
        return {
            'status': 'success',
            'operation': operation,
            'info': isp_operations[op_key]
        }
    else:
        return {
            'status': 'not_found',
            'operation': operation,
            'available_operations': list(isp_operations.keys()),
            'message': f'Unknown operation: {operation}. Available: {list(isp_operations.keys())}'
        }


def get_certification_requirements(cert_type: str) -> Dict[str, Any]:
    """
    Get wireless certification requirements.

    Certification types:
    - "FCC": US FCC certification
    - "CE": European CE certification
    - "SRRC": China SRRC certification
    - "IC": Canada IC certification
    - "MIC": Japan MIC certification

    Args:
        cert_type: Certification type

    Returns:
        Dictionary with certification requirements
    """
    # Certification requirements based on SDK documentation
    certification_data = {
        'fcc': {
            'name': 'FCC (US Federal Communications Commission)',
            'applicable_standards': [
                'FCC Part 15.247 (Bluetooth)',
                'FCC Part 15.209 (Spurious Emissions)'
            ],
            'test_items': [
                'Conducted emission (AC power)',
                'Radiated emission',
                'Bandwidth measurement',
                'Frequency stability',
                'Spurious emissions',
                'Output power'
            ],
            'required_documents': [
                'User manual',
                'Circuit schematic',
                'PCB layout',
                'Block diagram',
                'Test report',
                'Label sample',
                'Authorization letter'
            ],
            'lead_time': '4-6 weeks',
            'notes': 'FCC certification is required for products sold in the United States'
        },
        'ce': {
            'name': 'CE (European Conformity)',
            'applicable_directives': [
                'RED (Radio Equipment Directive) 2014/53/EU',
                'EMC Directive 2014/30/EU'
            ],
            'test_items': [
                'EMC test (EN 301 489)',
                'RF test (EN 300 328)',
                'Health safety (EN 62311)',
                'Spectrum efficiency'
            ],
            'required_documents': [
                'Technical construction file',
                'Test reports',
                'Declaration of Conformity',
                'User manual',
                'Label information'
            ],
            'lead_time': '3-4 weeks',
            'notes': 'CE marking is required for products sold in the EU'
        },
        'srrc': {
            'name': 'SRRC (China State Radio Regulation Committee)',
            'applicable_standards': [
                'YD/T 1214-2006',
                'YD/T 2408-2013'
            ],
            'test_items': [
                'Frequency range',
                'Output power',
                'Spurious emissions',
                'Adjacent channel power',
                'Frequency tolerance',
                'Modulation characteristics'
            ],
            'required_documents': [
                'Application form',
                'Business license',
                'Enterprise code certificate',
                'User manual',
                'Circuit schematic',
                'PCB layout',
                'Test report',
                'Color photos'
            ],
            'lead_time': '6-8 weeks',
            'notes': 'SRRC certification is mandatory for wireless products in China'
        },
        'ic': {
            'name': 'IC (Industry Canada)',
            'applicable_standards': [
                'RSS-247 (Bluetooth)',
                'ICES-003 (EMC)'
            ],
            'test_items': [
                'Radiated emission',
                'Conducted emission',
                'Bandwidth',
                'Output power',
                'Spurious emissions'
            ],
            'required_documents': [
                'Test report',
                'User manual',
                'Circuit schematic',
                'Label sample'
            ],
            'lead_time': '2-3 weeks',
            'notes': 'IC certification is required for products sold in Canada'
        },
        'mic': {
            'name': 'MIC (Japan Ministry of Internal Affairs and Communications)',
            'applicable_standards': [
                'Article 2 of the Radio Law',
                'MIC Ordinance No. 42'
            ],
            'test_items': [
                'Frequency stability',
                'Occupied bandwidth',
                'Spurious emission',
                'Output power',
                'Receiver characteristics'
            ],
            'required_documents': [
                'Application form',
                'Test report',
                'User manual',
                'Circuit diagram',
                'Block diagram',
                'Photos'
            ],
            'lead_time': '4-6 weeks',
            'notes': 'MIC certification is required for wireless products in Japan'
        }
    }

    # Normalize cert type
    cert_key = cert_type.lower().strip()

    if cert_key in certification_data:
        return {
            'status': 'success',
            'cert_type': cert_type,
            'requirements': certification_data[cert_key]
        }
    else:
        return {
            'status': 'not_found',
            'cert_type': cert_type,
            'available_certifications': list(certification_data.keys()),
            'message': f'Unknown certification: {cert_type}. Available: {list(certification_data.keys())}'
        }


def get_toolchain_setup(tool_name: str) -> Dict[str, Any]:
    """
    Get development tool setup guide.

    Supported tools:
    - "keil": Keil MDK-ARM
    - "gcc": GCC ARM Embedded
    - "jlink": J-Link debugger
    - "isp": ISP programming tool
    - "rtt": SEGGER RTT (Real-Time Transfer)

    Args:
        tool_name: Tool name

    Returns:
        Dictionary with setup instructions
    """
    # Toolchain setup guides
    toolchain_guides = {
        'keil': {
            'name': 'Keil MDK-ARM',
            'version': 'MDK 5.x',
            'download_url': 'https://www.keil.com/download/product/',
            'installation_steps': [
                '1. Download MDK-ARM installer',
                '2. Run installer as Administrator',
                '3. Select "MDK-ARM Core" and required device packs',
                '4. Complete installation',
                '5. Install B6x device pack (if available)'
            ],
            'configuration': [
                '1. Create new project',
                '2. Select B6x device',
                '3. Add startup files',
                '4. Configure target options (Flash, RAM)',
                '5. Select debugger (J-Link/U-LINK)',
                '6. Set programming algorithm'
            ],
            'notes': 'Use Keil version 5.30 or later for best compatibility'
        },
        'gcc': {
            'name': 'GCC ARM Embedded',
            'version': 'arm-none-eabi-gcc 10.x or later',
            'download_url': 'https://developer.arm.com/downloads/-/gnu-rm',
            'installation_steps': [
                '1. Download GCC ARM Embedded toolchain',
                '2. Extract to installation directory (e.g., C:\\GCC)',
                '3. Add bin directory to system PATH',
                '4. Verify installation: arm-none-eabi-gcc --version'
            ],
            'configuration': [
                '1. Set ARM_GCC_ROOT environment variable',
                '2. Configure Makefile or build script',
                '3. Specify include paths',
                '4. Set linker script',
                '5. Configure build options (-mcpu, -mthumb, etc.)'
            ],
            'notes': 'GCC is free and open-source alternative to Keil'
        },
        'jlink': {
            'name': 'SEGGER J-Link',
            'version': 'J-Link Software Pack V7.x',
            'download_url': 'https://www.segger.com/downloads/jlink/',
            'installation_steps': [
                '1. Download J-Link software pack',
                '2. Run installer',
                '3. Install J-Link drivers',
                '4. Connect J-Link debugger'
            ],
            'configuration': [
                '1. Launch J-Link Commander',
                '2. Select device interface (SWD/JTAG)',
                '3. Set target device speed',
                '4. Connect to target'
            ],
            'notes': 'J-Link supports SWD (2-wire) and JTAG (4-wire) interfaces'
        },
        'isp': {
            'name': 'ISP Programming Tool',
            'download_url': 'Contact vendor for ISP tool download',
            'installation_steps': [
                '1. Download ISP tool package',
                '2. Extract to installation directory',
                '3. Connect ISP dongle',
                '4. Install USB drivers (if prompted)',
                '5. Launch ISP tool'
            ],
            'configuration': [
                '1. Select COM port',
                '2. Set baudrate (115200 default)',
                '3. Configure auto-program options',
                '4. Set default firmware path'
            ],
            'notes': 'ISP tool requires UART connection to target'
        },
        'rtt': {
            'name': 'SEGGER RTT (Real-Time Transfer)',
            'version': 'Integrated in SDK',
            'location': 'drivers/src/RTT/',
            'setup_steps': [
                '1. Include RTT header in project',
                '2. Add RTT source files (SEGGER_RTT.c)',
                '3. Initialize RTT buffer',
                '4. Configure RTT channel',
                '5. Use RTT functions for debug output'
            ],
            'usage_example': '''
                // Include RTT
                #include "SEGGER_RTT.h"

                // Initialize
                SEGGER_RTT_Init();

                // Write to RTT
                SEGGER_RTT_WriteString(0, "Hello RTT!\\n");
            ''',
            'notes': 'RTT is faster than UART for debug output (no baudrate limit)'
        }
    }

    # Normalize tool name
    tool_key = tool_name.lower().strip().replace('-', '').replace('_', '')

    if tool_key in toolchain_guides:
        return {
            'status': 'success',
            'tool_name': tool_name,
            'setup': toolchain_guides[tool_key]
        }
    else:
        return {
            'status': 'not_found',
            'tool_name': tool_name,
            'available_tools': list(toolchain_guides.keys()),
            'message': f'Unknown tool: {tool_name}. Available: {list(toolchain_guides.keys())}'
        }


def search_production_docs(query: str) -> List[Dict[str, Any]]:
    """
    Search production-related documentation.

    Args:
        query: Search query string

    Returns:
        List of matching document sections
    """
    # Production document index (simulated for now, would use Whoosh in production)
    production_docs = [
        {
            'document': 'ISP Tool量产说明文档.docx',
            'section': 'ISP Tool Overview',
            'keywords': ['isp', 'programming', 'tool', '量产', '烧录'],
            'summary': 'Introduction to ISP programming tool for mass production'
        },
        {
            'document': 'ISP Tool量产说明文档.docx',
            'section': 'Production Programming Steps',
            'keywords': ['production', 'programming', 'steps', 'workflow'],
            'summary': 'Step-by-step guide for production programming workflow'
        },
        {
            'document': 'JFlash配置说明.docx',
            'section': 'JFlash Configuration',
            'keywords': ['jflash', 'configuration', 'setup', 'flash'],
            'summary': 'JFlash configuration for B6x chip programming'
        },
        {
            'document': 'bxISP_Programmer_User_Guide_V1.0_CN.docx',
            'section': 'ISP Programmer User Guide',
            'keywords': ['programmer', 'user guide', 'isp', 'manual'],
            'summary': 'Complete user manual for ISP programmer'
        },
        {
            'document': 'B6x无线认证指导.docx',
            'section': 'FCC Certification',
            'keywords': ['fcc', 'certification', 'wireless', 'us'],
            'summary': 'FCC certification requirements and procedures'
        },
        {
            'document': 'B6x无线认证指导.docx',
            'section': 'CE Certification',
            'keywords': ['ce', 'certification', 'european', 'red'],
            'summary': 'CE RED certification requirements for EU market'
        },
        {
            'document': 'B6x无线认证指导.docx',
            'section': 'SRRC Certification',
            'keywords': ['srrc', 'certification', 'china', 'wireless'],
            'summary': 'SRRC certification requirements for China market'
        }
    ]

    # Simple keyword matching (would use Whoosh in production)
    query_lower = query.lower()
    matches = []

    for doc in production_docs:
        # Check if query matches keywords or summary
        keyword_match = any(query_lower in kw.lower() for kw in doc['keywords'])
        summary_match = query_lower in doc['summary'].lower()
        section_match = query_lower in doc['section'].lower()

        if keyword_match or summary_match or section_match:
            matches.append(doc)

    return matches


# ============================================================================
# Tool Registration (for MCP server)
# ============================================================================

def register_domain5_tools(tool_registry):
    """
    Register all Domain 5 tools with the MCP server.

    Args:
        tool_registry: Tool registry instance
    """
    tools = [
        ('get_production_guide', get_production_guide),
        ('get_jflash_config', get_jflash_config),
        ('get_isp_tool_usage', get_isp_tool_usage),
        ('get_certification_requirements', get_certification_requirements),
        ('get_toolchain_setup', get_toolchain_setup),
        ('search_production_docs', search_production_docs)
    ]

    for tool_name, tool_func in tools:
        tool_registry.register(tool_name, tool_func)
        logger.info(f"Registered Domain 5 tool: {tool_name}")


# ============================================================================
# Convenience Functions
# ============================================================================

def get_all_domain5_tools() -> Dict[str, callable]:
    """Get all Domain 5 tool functions."""
    return {
        'get_production_guide': get_production_guide,
        'get_jflash_config': get_jflash_config,
        'get_isp_tool_usage': get_isp_tool_usage,
        'get_certification_requirements': get_certification_requirements,
        'get_toolchain_setup': get_toolchain_setup,
        'search_production_docs': search_production_docs
    }


if __name__ == "__main__":
    import json

    # Test tools
    print("Testing Domain 5 Tools:")
    print("="*60)

    # Test 1: Production guide
    print("\n1. Production Guide (ISP):")
    result = get_production_guide('isp')
    print(json.dumps(result, indent=2, ensure_ascii=False))

    # Test 2: JFlash config
    print("\n2. JFlash Config (QFN32):")
    result = get_jflash_config('QFN32')
    print(json.dumps(result, indent=2, ensure_ascii=False))

    # Test 3: Certification requirements
    print("\n3. Certification Requirements (FCC):")
    result = get_certification_requirements('FCC')
    print(json.dumps(result, indent=2, ensure_ascii=False))

    # Test 4: Toolchain setup
    print("\n4. Toolchain Setup (Keil):")
    result = get_toolchain_setup('Keil')
    print(json.dumps(result, indent=2, ensure_ascii=False))

    # Test 5: Search production docs
    print("\n5. Search Production Docs ('certification'):")
    results = search_production_docs('certification')
    print(json.dumps(results, indent=2, ensure_ascii=False))
