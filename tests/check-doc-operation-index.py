#!/usr/bin/env python3
"""Check that the bilingual documentation tracks registered builtins."""
import json
import re
from pathlib import Path

root = Path(__file__).resolve().parent.parent
entries = json.loads((root / 'docs-site/_data/operations.json').read_text())
details = json.loads((root / 'docs-site/_data/operation_details.json').read_text())
names = [item['name'] for item in entries]
assert len(names) == len(set(names)), 'duplicate operation'
registered = set()
for source in (root / 'numericApp/src').glob('*.cpp'):
    registered.update(re.findall(r'Metadata(?:\s+\w+)?\s*\{\s*"([a-z][a-z0-9_]*)"', source.read_text()))
assert set(names) == registered, f'missing: {sorted(registered - set(names))}; extra: {sorted(set(names) - registered)}'
assert set(details) == registered, 'detailed reference does not match registered operations'
for item in entries:
    for field in ('inputs_en', 'inputs_ja', 'result_en', 'result_ja'):
        assert item[field].strip(), f"{item['name']}: missing {field}"
    assert bool(item.get('args_en')) == bool(item.get('args_ja')), f"{item['name']}: untranslated arguments"
    assert item['interface'] in ('scalar', 'array', 'asub')
    assert (root / 'docs' / item['doc']).is_file(), f"{item['name']}: missing reference"
    detail = details[item['name']]
    for field in ('types', 'example_inputs', 'example_output', 'error_en', 'error_ja', 'output_shape'):
        assert detail.get(field), f"{item['name']}: missing {field}"
print(f'{len(names)} registered operations documented in English and Japanese')
