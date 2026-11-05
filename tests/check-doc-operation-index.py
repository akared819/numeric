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

native_entries = []
for filename in ('native_details.json', 'native_core_details.json'):
    native_entries.extend(json.loads((root / 'docs-site/_data' / filename).read_text()))
native_names = [item['name'] for item in native_entries]
assert len(native_names) == len(set(native_names)), 'duplicate native API'
exported = set()
for header in ('numeric.h', 'numericData.h'):
    text = (root / 'numericApp/src' / header).read_text()
    exported.update(re.findall(r'NUMERIC_API\s+(?:Result|void|bool)\s+(\w+)\s*\(', text))
missing_native = exported - set(native_names)
assert not missing_native, f'undocumented exported functions: {sorted(missing_native)}'
for item in native_entries:
    for field in ('signature', 'parameters_en', 'parameters_ja', 'returns_en',
                  'returns_ja', 'errors_en', 'errors_ja', 'example'):
        assert item.get(field), f"{item['name']}: missing {field}"
print(f'{len(native_entries)} native API entries have bilingual details and examples')

asub_entries = json.loads((root / 'docs-site/_data/asub_details.json').read_text())
asub_names = [item['name'] for item in asub_entries]
assert len(asub_names) == len(set(asub_names)), 'duplicate aSub function'
dbd = (root / 'numericApp/src/numericSupport.dbd').read_text()
registered_asub = set(re.findall(r'^function\((\w+)\)', dbd, re.MULTILINE))
assert set(asub_names) == registered_asub, 'aSub documentation does not match registrations'
for item in asub_entries:
    for field in ('inputs_en', 'inputs_ja', 'outputs_en', 'outputs_ja',
                  'example_inputs', 'example_output'):
        assert item.get(field), f"{item['name']}: missing {field}"
    assert item['operation'] in details, f"{item['name']}: missing operation"
    example = root / 'examples' / item['db']
    assert example.is_file(), f"{item['name']}: missing example"
    if not item.get('adapt_example'):
        assert item['name'] in example.read_text(), f"{item['name']}: incorrect example"
print(f'{len(asub_entries)} aSub functions have bilingual details and examples')
