"""
Mesh Parser for B6x SDK
=========================

解析 B6x SDK 的 Mesh 模块，提取：
1. Mesh API 函数
2. Mesh 错误码
3. Mesh Model 定义

Author: B6x MCP Server Team
Version: 1.0.0
"""

import re
from pathlib import Path
from typing import Dict, List, Any, Optional
from dataclasses import dataclass, field


@dataclass
class MeshAPI:
    """Mesh API 函数定义"""
    name: str
    return_type: str
    parameters: List[Dict[str, str]]
    brief: str
    header_file: str
    line_number: int


@dataclass
class MeshErrorCode:
    """Mesh 错误码定义"""
    code: int
    hex_code: str
    name: str
    group: str  # PROTOCOL, PROVISIONING, INTERNAL, LPN, MDL
    description: str


@dataclass
class MeshModel:
    """Mesh Model 定义"""
    name: str
    model_id: int
    type: str  # GEN_ONOFF, GEN_LVL, LIGHT_LN, etc.
    header_file: str
    states: List[str]
    description: str


class MeshAPIParser:
    """Mesh API 解析器"""

    def __init__(self, mesh_api_dir: str):
        self.mesh_api_dir = Path(mesh_api_dir)
        self.apis: List[MeshAPI] = []

    def parse(self) -> List[MeshAPI]:
        """解析 Mesh API 头文件"""
        self.apis = []

        # 主要 API 文件
        mesh_h = self.mesh_api_dir / "mesh.h"
        if mesh_h.exists():
            self._parse_mesh_h(mesh_h)

        return self.apis

    def _parse_mesh_h(self, file_path: Path):
        """解析 mesh.h 文件"""
        content = file_path.read_text(encoding='utf-8', errors='ignore')

        # 匹配函数声明模式
        # 示例: typedef void (*mesh_djob_cb)(void* p_djob);
        # 示例: typedef void (*mesh_timer_cb)(void *p_timer);

        # 提取函数指针类型定义
        func_ptr_pattern = r'typedef\s+(\w+)\s+\(\*([\w_]+)\)\s*\(([^)]*)\)\s*;'
        for match in re.finditer(func_ptr_pattern, content):
            return_type = match.group(1)
            name = match.group(2)
            params_str = match.group(3)

            # 解析参数
            parameters = []
            if params_str.strip() and params_str.strip() != 'void':
                for param in params_str.split(','):
                    param = param.strip()
                    if param:
                        parameters.append({
                            "type": "unknown",
                            "name": param
                        })

            api = MeshAPI(
                name=name,
                return_type=f"{return_type} (*)",
                parameters=parameters,
                brief=f"Mesh {name} callback",
                header_file="mesh.h",
                line_number=0
            )
            self.apis.append(api)

        # 匹配结构体定义
        # 示例: typedef struct mesh_djob
        struct_pattern = r'typedef\s+struct\s+(\w+)\s*\{([^}]*)\}\s*(\w+)\s*;'
        for match in re.finditer(struct_pattern, content, re.DOTALL):
            struct_body = match.group(2)
            struct_name = match.group(3)

            # 提取字段
            fields = self._extract_struct_fields(struct_body)

            api = MeshAPI(
                name=struct_name,
                return_type="struct",
                parameters=fields,
                brief=f"Mesh {struct_name} structure",
                header_file="mesh.h",
                line_number=0
            )
            self.apis.append(api)

        # 匹配宏定义
        # 示例: #define MESH_BUF_ENV_SIZE (32)
        macro_pattern = r'#define\s+(\w+)\s*\(\s*([^)]+)\s*\)'
        for match in re.finditer(macro_pattern, content):
            macro_name = match.group(1)
            macro_value = match.group(2).strip()

            api = MeshAPI(
                name=macro_name,
                return_type=macro_value,
                parameters=[],
                brief=f"Mesh configuration: {macro_name}",
                header_file="mesh.h",
                line_number=0
            )
            self.apis.append(api)

    def _extract_struct_fields(self, struct_body: str) -> List[Dict[str, str]]:
        """从结构体中提取字段"""
        fields = []

        # 简单的字段匹配
        field_pattern = r'///[^\n]*?\n\s+(\w+)\s+(\w+)\s*(?:\[([^\]]+)\])?\s*;'
        for match in re.finditer(field_pattern, struct_body):
            field_type = match.group(1)
            field_name = match.group(2)
            array_size = match.group(3) if match.group(3) else ""

            field_info = {
                "type": field_type,
                "name": field_name
            }
            if array_size:
                field_info["array_size"] = array_size

            fields.append(field_info)

        return fields


class MeshErrorCodeParser:
    """Mesh 错误码解析器"""

    def __init__(self, mesh_api_dir: str):
        self.mesh_api_dir = Path(mesh_api_dir)
        self.error_codes: Dict[int, MeshErrorCode] = {}

    def parse(self) -> Dict[int, MeshErrorCode]:
        """解析 Mesh 错误码"""
        error_h = self.mesh_api_dir / "mesh_err.h"
        if error_h.exists():
            return self._parse_mesh_err_h(error_h)

        return {}

    def _parse_mesh_err_h(self, file_path: Path) -> Dict[int, MeshErrorCode]:
        """解析 mesh_err.h 文件"""
        content = file_path.read_text(encoding='utf-8', errors='ignore')

        # 提取错误码枚举
        # 示例: MESH_ERR_INVALID_ADDR = MESH_ERR_(PROTOCOL, 0x01)

        # 错误码组定义
        group_patterns = {
            'MESH_ERR_PROTOCOL_CODE': 'PROTOCOL',
            'MESH_ERR_PROVISIONING_CODE': 'PROVISIONING',
            'MESH_ERR_INTERNAL_CODE': 'INTERNAL',
            'MESH_ERR_LPN_CODE': 'LPN',
            'MESH_ERR_MDL_CODE': 'MDL'
        }

        # 匹配错误码定义
        error_pattern = r'(\w+)\s*=\s*MESH_ERR_\((\w+),\s*(0x[0-9A-Fa-f]+)\)'
        for match in re.finditer(error_pattern, content):
            error_name = match.group(1)
            group_code = match.group(2)
            sub_error = int(match.group(3), 16)

            # 计算完整错误码
            # 错误码 = (group_code << 5) | (sub_error & 0x1F)
            group_shift = self._get_group_shift(group_code)
            full_code = (group_shift << 5) | (sub_error & 0x1F)

            # 查找描述注释
            brief = self._find_error_brief(content, match.start())

            error = MeshErrorCode(
                code=full_code,
                hex_code=f"0x{full_code:04X}",
                name=error_name,
                group=group_patterns.get(group_code, group_code),
                description=brief
            )
            self.error_codes[full_code] = error

        return self.error_codes

    def _get_group_shift(self, group_code: str) -> int:
        """获取错误码组的 shift 值"""
        shifts = {
            'MESH_ERR_PROTOCOL_CODE': 0x01,
            'MESH_ERR_PROVISIONING_CODE': 0x02,
            'MESH_ERR_INTERNAL_CODE': 0x03,
            'MESH_ERR_LPN_CODE': 0x04,
            'MESH_ERR_MDL_CODE': 0x05,
        }
        return shifts.get(group_code, 0)

    def _find_error_brief(self, content: str, start_pos: int) -> str:
        """查找错误码的简短描述"""
        # 查找前面的注释
        lines_before = content[:start_pos].split('\n')
        brief = ""

        # 向前查找注释行
        for line in reversed(lines_before[-5:]):  # 只看最近5行
            line = line.strip()
            if line.startswith('///'):
                brief = line[3:].strip()
            elif line.startswith('/*') or line.startswith('*'):
                break

        return brief


class MeshModelParser:
    """Mesh Model 解析器"""

    def __init__(self, mesh_model_dir: str):
        self.mesh_model_dir = Path(mesh_model_dir)
        self.models: List[MeshModel] = []

    def parse(self) -> List[MeshModel]:
        """解析 Mesh Model 头文件"""
        self.models = []

        # 解析所有模型头文件
        for header_file in self.mesh_model_dir.glob("*.h"):
            self._parse_model_header(header_file)

        return self.models

    def _parse_model_header(self, file_path: Path):
        """解析模型头文件"""
        content = file_path.read_text(encoding='utf-8', errors='ignore')

        # 提取模型名称和类型
        # 从文件名推断模型类型
        model_type = self._infer_model_type(file_path.stem)

        # 提取状态枚举
        states = self._extract_states(content)

        # 提取模型描述
        description = self._extract_description(content)

        model = MeshModel(
            name=file_path.stem,
            model_id=0,
            type=model_type,
            header_file=str(file_path.relative_to(self.mesh_model_dir.parent.parent)),
            states=states,
            description=description
        )
        self.models.append(model)

    def _infer_model_type(self, filename: str) -> str:
        """从文件名推断模型类型"""
        if 'gen' in filename.lower():
            return 'GEN_MODEL'
        elif 'light' in filename.lower():
            return 'LIGHT_MODEL'
        elif 'sens' in filename.lower():
            return 'SENSOR_MODEL'
        elif 'tscn' in filename.lower():
            return 'TIME_SCENE_MODEL'
        else:
            return 'UNKNOWN'

    def _extract_states(self, content: str) -> List[str]:
        """提取状态枚举"""
        states = []

        # 匹配状态枚举值
        # 示例: MM_STATE_GEN_ONOFF = 0
        state_pattern = r'MM_STATE_[A-Z_]+\s*=\s*(\d+)'
        for match in re.finditer(state_pattern, content):
            states.append(match.group(0))

        return states

    def _extract_description(self, content: str) -> str:
        """提取模型描述"""
        # 查找文件开头的描述注释
        lines = content.split('\n')[:20]  # 只看前20行

        description = ""
        in_block_comment = False

        for line in lines:
            stripped = line.strip()

            if '/*' in stripped:
                in_block_comment = True
                continue

            if '*/' in stripped:
                in_block_comment = False
                continue

            if in_block_comment:
                # 提取注释内容
                if '*' in stripped:
                    desc_line = stripped.split('*', 1)[-1].strip()
                    if desc_line:
                        description += desc_line + " "
            elif stripped.startswith('///'):
                desc_line = stripped[3:].strip()
                if desc_line:
                    description += desc_line + " "
            elif '@brief' in stripped:
                parts = stripped.split('@brief')
                if len(parts) > 1:
                    description += parts[1].strip() + " "

        return description.strip()


def parse_mesh_module(mesh_dir: str, sdk_path: str) -> Dict[str, Any]:
    """
    解析整个 Mesh 模块

    Returns:
        包含 Mesh APIs, 错误码和模型的字典
    """
    mesh_path = Path(sdk_path) / "mesh"

    if not mesh_path.exists():
        return {
            "apis": [],
            "error_codes": {},
            "models": [],
            "status": "Mesh directory not found"
        }

    # 解析 Mesh API
    api_parser = MeshAPIParser(str(mesh_path / "api"))
    apis = api_parser.parse()

    # 解析 Mesh 错误码
    error_parser = MeshErrorCodeParser(str(mesh_path / "api"))
    error_codes = error_parser.parse()

    # 解析 Mesh Models
    model_parser = MeshModelParser(str(mesh_path / "model" / "api"))
    models = model_parser.parse()

    return {
        "apis": apis,
        "error_codes": error_codes,
        "models": models,
        "status": "success"
    }


# 导出函数
def extract_mesh_apis(mesh_dir: str) -> List[Dict[str, Any]]:
    """提取 Mesh API（供知识图谱构建器使用）"""
    parser = MeshAPIParser(mesh_dir)
    apis = parser.parse()

    return [
        {
            "name": api.name,
            "return_type": api.return_type,
            "parameters": api.parameters,
            "brief": api.brief,
            "header_file": api.header_file
        }
        for api in apis
    ]


def extract_mesh_error_codes(mesh_dir: str) -> List[Dict[str, Any]]:
    """提取 Mesh 错误码（供知识图谱构建器使用）"""
    parser = MeshErrorCodeParser(mesh_dir)
    error_codes = parser.parse()

    return [
        {
            "code": err.code,
            "hex_code": err.hex_code,
            "name": err.name,
            "group": err.group,
            "description": err.description
        }
        for err in error_codes.values()
    ]


def extract_mesh_models(mesh_model_dir: str) -> List[Dict[str, Any]]:
    """提取 Mesh 模型（供知识图谱构建器使用）"""
    parser = MeshModelParser(mesh_model_dir)
    models = parser.parse()

    return [
        {
            "name": model.name,
            "model_id": model.model_id,
            "type": model.type,
            "header_file": model.header_file,
            "states": model.states,
            "description": model.description
        }
        for model in models
    ]


if __name__ == "__main__":
    # 测试代码
    sdk_path = "D:/svn/bxx_DragonC1/sdk6"

    result = parse_mesh_module("mesh", sdk_path)

    print("="*70)
    print("Mesh Module Parsing Result")
    print("="*70)

    print(f"\nStatus: {result['status']}")
    print(f"APIs: {len(result['apis'])}")
    print(f"Error Codes: {len(result['error_codes'])}")
    print(f"Models: {len(result['models'])}")

    if result['apis']:
        print("\n--- Sample APIs ---")
        for api in result['apis'][:3]:
            print(f"  - {api['name']}: {api['brief']}")

    if result['error_codes']:
        print("\n--- Sample Error Codes ---")
        for err in list(result['error_codes'].values())[:3]:
            print(f"  - {err['hex_code']}: {err['name']} ({err['group']})")

    if result['models']:
        print("\n--- Sample Models ---")
        for model in result['models'][:3]:
            print(f"  - {model['name']}: {model['type']} ({len(model['states'])} states)")
