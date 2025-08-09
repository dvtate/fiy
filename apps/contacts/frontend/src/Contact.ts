import ICAL from "ical.js";



const vcardPropertyTypes =
{
    "fn": "text",
    "bday": "date",
    "anniversary": "text",
    "gender": "sex",
    "lang": "language-tag",
    "tel": "uri",
    "geo": "geo",
    "key": "uri",
    "photo": "uri",
    "impp": "uri",
    "logo": "img",
    "member": "uri",
    "related": "uri",
    "rev": "timestamp",
    "sound": "uri",
    "uid": "uri",
    "url": "uri",
    "fburl": "uri",
    "caladruri": "uri",
    "caluri": "uri",
    "source": "uri",
    "adr": "text",
    "n": "name"
};


class VCard {
    public static readonly BEGIN_TOKEN = "BEGIN:VCARD";
    public static readonly END_TOKEN = "END:VCARD";

    version: '2.1' | '3.0' | '4.0' | string = '4.0';

    properties: VCardProperty[] = [];

    constructor(fields) {
        fields.forEach(f => {
            if (f)
        })
    }

    addProperty(property: VCardProperty) {
        if (property.name === 'VERSION')
            this.version = property.value;
        this.properties.push(property);
    }

    static parse()

    cardString() {

    }
};
