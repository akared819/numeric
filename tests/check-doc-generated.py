#!/usr/bin/env python3
"""Check generated reference anchors in both languages."""
import json
import sys
from collections import Counter
from html.parser import HTMLParser
from pathlib import Path

root = Path(__file__).resolve().parent.parent
site = Path(sys.argv[1])

class Page(HTMLParser):
    def __init__(self):
        super().__init__()
        self.ids = []
        self.anchors = []

    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        if attrs.get('id'):
            self.ids.append(attrs['id'])
        if tag == 'a' and attrs.get('href', '').startswith('#'):
            self.anchors.append(attrs['href'][1:])

operation_names = [x['name'] for x in json.loads((root / 'docs-site/_data/operations.json').read_text())]
asub_names = [x['name'].lower() for x in json.loads((root / 'docs-site/_data/asub_details.json').read_text())]
native_names = []
for filename in ('native_details.json', 'native_core_details.json'):
    native_names += [x['name'].lower().replace(':', '') for x in json.loads((root / 'docs-site/_data' / filename).read_text())]
for path in ('reference/index.html', 'ja/reference/index.html'):
    page = Page()
    page.feed((site / path).read_text())
    duplicates = [key for key, count in Counter(page.ids).items() if count > 1]
    assert not duplicates, f'{path}: duplicate IDs {duplicates}'
    missing = (set(page.anchors) | set(operation_names) | set(asub_names) | set(native_names)) - set(page.ids)
    assert not missing, f'{path}: missing anchors {sorted(missing)}'
    print(f'{path}: all operation, aSub, native and local links resolve')
