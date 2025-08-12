## Model
1. Use ICAL.js to parse vcard
    - altho it seems super buggy
2. Pass output to VCProperty.create
    - or Skip step 1 and make parseVCard(str): VCProperty[] (requires card+property splitter)
3. use VCProperty.showHtml and VCProperty.editHtml to generate UIs.

## View
### Viewer
- sort properties for card, by name, relevance, etc. and display them

### Editor
- add property button, when clicked adds blank vcproperty field