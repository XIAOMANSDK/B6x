#!/usr/bin/env python3
"""
Data Quality Report Generator for B6x MCP Server
===================================================

Analyzes generated data files and produces a quality report showing:
- Which data is from Excel parsing vs default generation
- Source data coverage statistics
- Parsing success rates
- Data completeness metrics

Usage:
    python scripts/generate_data_quality_report.py

Author: B6x MCP Server Team
Version: 1.0.0
"""

import sys
import json
import logging
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Any
from collections import defaultdict

# Add src to path for imports
sys.path.insert(0, str(Path(__file__).parent.parent / "src"))

logging.basicConfig(
    level=logging.INFO,
    format='%(levelname)s: %(message)s'
)
logger = logging.getLogger(__name__)


class DataQualityAnalyzer:
    """Analyze data quality across all generated files."""

    def __init__(self, data_dir: Path):
        """Initialize analyzer with data directory."""
        self.data_dir = data_dir
        self.results = {
            "constraints": {},
            "domain": {},
            "relations": {},
            "overall": {}
        }

    def analyze_all(self) -> Dict[str, Any]:
        """Run all analyses and generate comprehensive report."""
        logger.info("Starting data quality analysis...")

        # Analyze constraint files
        self.analyze_constraints()

        # Analyze domain files
        self.analyze_domain_data()

        # Analyze relation files
        self.analyze_relations()

        # Calculate overall metrics
        self.calculate_overall_metrics()

        return self.results

    def analyze_constraints(self):
        """Analyze constraint files for data quality."""
        constraints_dir = self.data_dir / "constraints"
        if not constraints_dir.exists():
            logger.warning(f"Constraints directory not found: {constraints_dir}")
            return

        constraint_files = list(constraints_dir.glob("*.json"))
        exclude_files = {"build_report.json"}

        for file_path in constraint_files:
            if file_path.name in exclude_files:
                continue

            try:
                with open(file_path, 'r', encoding='utf-8') as f:
                    data = json.load(f)

                file_analysis = {
                    "file_name": file_path.name,
                    "file_size_kb": file_path.stat().st_size / 1024,
                    "data_source": data.get("data_source", "unknown"),
                    "constraint_type": data.get("constraint_type", "unknown"),
                    "domain": data.get("domain", "unknown"),
                    "has_warnings": "warning" in data,
                    "is_default_data": data.get("data_source") == "default_generation",
                    "entry_count": self._count_entries(data),
                    "metadata": {
                        "version": data.get("version"),
                        "last_updated": data.get("last_updated"),
                        "source_file": data.get("source_file")
                    }
                }

                self.results["constraints"][file_path.name] = file_analysis

            except Exception as e:
                logger.error(f"Error analyzing {file_path.name}: {e}")
                self.results["constraints"][file_path.name] = {
                    "file_name": file_path.name,
                    "error": str(e)
                }

    def analyze_domain_data(self):
        """Analyze domain data files."""
        domain_dir = self.data_dir / "domain"
        if not domain_dir.exists():
            logger.warning(f"Domain directory not found: {domain_dir}")
            return

        for domain_subdir in domain_dir.iterdir():
            if not domain_subdir.is_dir():
                continue

            domain_name = domain_subdir.name
            self.results["domain"][domain_name] = {
                "subdirectories": {},
                "total_files": 0,
                "total_size_kb": 0
            }

            for file_path in domain_subdir.glob("*.json"):
                try:
                    with open(file_path, 'r', encoding='utf-8') as f:
                        data = json.load(f)

                    file_analysis = {
                        "file_name": file_path.name,
                        "file_size_kb": file_path.stat().st_size / 1024,
                        "entry_count": self._count_entries(data),
                        "has_metadata": "metadata" in file_path.name or "_metadata" in file_path.name
                    }

                    self.results["domain"][domain_name]["subdirectories"][file_path.name] = file_analysis
                    self.results["domain"][domain_name]["total_files"] += 1
                    self.results["domain"][domain_name]["total_size_kb"] += file_analysis["file_size_kb"]

                except Exception as e:
                    logger.error(f"Error analyzing {file_path}: {e}")

    def analyze_relations(self):
        """Analyze relation files."""
        relations_dir = self.data_dir / "relations"
        if not relations_dir.exists():
            logger.warning(f"Relations directory not found: {relations_dir}")
            return

        for file_path in relations_dir.glob("*.json"):
            try:
                with open(file_path, 'r', encoding='utf-8') as f:
                    data = json.load(f)

                relation_count = 0
                if isinstance(data, list):
                    relation_count = len(data)
                elif isinstance(data, dict):
                    relation_count = len(data.get("relations", []))

                self.results["relations"][file_path.name] = {
                    "file_name": file_path.name,
                    "file_size_kb": file_path.stat().st_size / 1024,
                    "relation_count": relation_count
                }

            except Exception as e:
                logger.error(f"Error analyzing {file_path}: {e}")

    def calculate_overall_metrics(self):
        """Calculate overall quality metrics."""
        # Count files by data source
        source_counts = defaultdict(int)
        for file_name, analysis in self.results["constraints"].items():
            if "data_source" in analysis:
                source_counts[analysis["data_source"]] += 1

        # Calculate default data percentage
        total_constraints = len(self.results["constraints"])
        default_count = source_counts.get("default_generation", 0)
        default_percentage = (default_count / total_constraints * 100) if total_constraints > 0 else 0

        # Count domain files
        total_domain_files = sum(
            domain_data.get("total_files", 0)
            for domain_data in self.results["domain"].values()
        )

        # Count total relations
        total_relations = sum(
            relation_data.get("relation_count", 0)
            for relation_data in self.results["relations"].values()
        )

        self.results["overall"] = {
            "total_constraint_files": total_constraints,
            "total_domain_files": total_domain_files,
            "total_relation_files": len(self.results["relations"]),
            "total_relations": total_relations,
            "data_source_breakdown": dict(source_counts),
            "default_data_percentage": round(default_percentage, 2),
            "quality_score": self._calculate_quality_score(default_percentage)
        }

    def _count_entries(self, data: Dict[str, Any]) -> int:
        """Count entries in data based on its structure."""
        count = 0

        # Check for common entry keys
        for key in ["total_pins", "total_regions", "total_modes", "total_features",
                    "total_issues", "total_boundaries", "total_size_kb"]:
            if key in data:
                return data[key]

        # Check for list-based entries
        for key in ["pins", "regions", "modes", "features", "brands",
                    "boundaries", "issues", "functions", "apis"]:
            if key in data and isinstance(data[key], list):
                return len(data[key])

        return count

    def _calculate_quality_score(self, default_percentage: float) -> str:
        """Calculate overall quality score based on default data percentage."""
        if default_percentage == 0:
            return "EXCELLENT"
        elif default_percentage < 20:
            return "GOOD"
        elif default_percentage < 50:
            return "FAIR"
        else:
            return "POOR"

    def generate_report(self) -> str:
        """Generate human-readable quality report."""
        report = []
        report.append("=" * 70)
        report.append("B6x MCP Server - Data Quality Report")
        report.append("=" * 70)
        report.append(f"Generated: {datetime.now().isoformat()}")
        report.append("")

        # Overall Summary
        overall = self.results.get("overall", {})
        report.append("OVERALL SUMMARY")
        report.append("-" * 70)
        report.append(f"Total Constraint Files:  {overall.get('total_constraint_files', 0)}")
        report.append(f"Total Domain Files:      {overall.get('total_domain_files', 0)}")
        report.append(f"Total Relation Files:    {overall.get('total_relation_files', 0)}")
        report.append(f"Total Relations:         {overall.get('total_relations', 0)}")
        report.append("")
        report.append(f"Quality Score:           {overall.get('quality_score', 'UNKNOWN')}")
        report.append(f"Default Data Percentage: {overall.get('default_data_percentage', 0)}%")
        report.append("")

        # Data Source Breakdown
        source_breakdown = overall.get("data_source_breakdown", {})
        if source_breakdown:
            report.append("DATA SOURCE BREAKDOWN")
            report.append("-" * 70)
            for source, count in source_breakdown.items():
                pct = (count / overall.get('total_constraint_files', 1)) * 100
                report.append(f"  {source:25} {count:3} files ({pct:5.1f}%)")
            report.append("")

        # Constraint Files Detail
        if self.results.get("constraints"):
            report.append("CONSTRAINT FILES DETAIL")
            report.append("-" * 70)
            for file_name, analysis in self.results["constraints"].items():
                if "error" in analysis:
                    report.append(f"  [ERROR] {file_name}: {analysis['error']}")
                else:
                    source_indicator = "⚠️ DEFAULT" if analysis.get("is_default_data") else "✅ PARSED"
                    report.append(f"  {source_indicator} {file_name}")
                    report.append(f"             Type: {analysis.get('constraint_type', 'N/A')}")
                    report.append(f"             Entries: {analysis.get('entry_count', 0)}")
                    if analysis.get("has_warnings"):
                        report.append(f"             Warning: Default data - may not reflect actual specs")
            report.append("")

        # Domain Files Summary
        if self.results.get("domain"):
            report.append("DOMAIN FILES SUMMARY")
            report.append("-" * 70)
            for domain_name, domain_data in self.results["domain"].items():
                total_files = domain_data.get("total_files", 0)
                total_size = domain_data.get("total_size_kb", 0)
                report.append(f"  {domain_name:15} {total_files:3} files, {total_size:8.1f} KB")
            report.append("")

        # Relation Files Summary
        if self.results.get("relations"):
            report.append("RELATION FILES SUMMARY")
            report.append("-" * 70)
            for file_name, relation_data in self.results["relations"].items():
                count = relation_data.get("relation_count", 0)
                report.append(f"  {file_name:40} {count:6} relations")
            report.append("")

        # Recommendations
        report.append("RECOMMENDATIONS")
        report.append("-" * 70)

        default_pct = overall.get('default_data_percentage', 0)
        if default_pct > 0:
            report.append(f"⚠️  {default_pct:.1f}% of constraint data is from default generation")
            report.append("   Action items:")
            for file_name, analysis in self.results["constraints"].items():
                if analysis.get("is_default_data"):
                    report.append(f"     - Verify {file_name} with actual Excel source file")
        else:
            report.append("✅ All constraint data successfully parsed from Excel files")

        report.append("")

        return "\n".join(report)


def find_data_dir() -> Path:
    """Find the data directory."""
    # Start from script location
    script_dir = Path(__file__).parent
    mcp_root = script_dir.parent
    data_dir = mcp_root / "data"

    if not data_dir.exists():
        # Try relative to script
        data_dir = script_dir / "data"

    return data_dir


def main():
    """Main entry point."""
    data_dir = find_data_dir()

    if not data_dir.exists():
        logger.error(f"Data directory not found: {data_dir}")
        return 1

    print(f"Analyzing data directory: {data_dir}")
    print()

    # Run analysis
    analyzer = DataQualityAnalyzer(data_dir)
    analyzer.analyze_all()

    # Generate and print report
    report = analyzer.generate_report()
    print(report)

    # Save JSON report
    report_path = data_dir / "data_quality_report.json"
    json_report = {
        "generated_at": datetime.now().isoformat(),
        "data_dir": str(data_dir),
        "analysis": analyzer.results
    }

    with open(report_path, 'w', encoding='utf-8') as f:
        json.dump(json_report, f, indent=2, ensure_ascii=False)

    print(f"JSON report saved to: {report_path}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
