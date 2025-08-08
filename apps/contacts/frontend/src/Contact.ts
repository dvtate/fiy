import ICAL from "ical.js";

const VCardFieldNames = [
    'VERSION',          // vcard version
    'ADR',              // address
    'AGENT',                // person who acts on their behalf (ie - secretary)
    'BDAY',             // birthday
    'CATEGORIES',            // tags that describe object
    'CLASS',                 // describes sensitivity contact info
    'LABEL',            // what to put on a shipping label
    'EMAIL',            // email address
    'FN',               // Full name
    'GEO',                   // lat+long location
    'KEY',              // cryptographic key
    'LOGO',                  // logo image
    'MAILER',                // email program used
    'N',                // structured name
// Family Name, Given Name, Additional/Middle Names, Honorific Prefixes, and Honorific Suffixes

    'NICKNAME',         // nickname
    'NOTE',             // additional info
    'ORG',              // company/organization
    'PHOTO',            // pfp
    'PRODID',           // product that created the vcard (ie - fediy)
    'REV',              // timestamp last time vcard was updated
    'ROLE',             // role/occupation/business category (ie - within ORG)
    'SORT-STRING',      // used when application sorts, instead use SORT-AS parameter of N/ORG
    'SOUND',            // name pronounciation sound
    'TEL',              // phone number
    'TZ',               // Timezone (ie - America/New York
    'TITLE',            // job title
    'URL',              // website
];

const vcardProperties = {
    'ADR': {
        name: 'Address',
        description: 'Physical delivery address',
        type: 'address',
        versions: true,
        types: ['home', 'work'],
    },
    'AGENT': {
        name: 'Agent',
        description: 'Person who will act on their behalf',
        type: 'uri',
        versions: ['2.1', '3.0'],
    },
    'ANNIVERSARY': {
        name: 'Anniversary',
        description: 'Anniversary Date',
        type: 'date-and-or-time',
        versions: ['4.0'],
    },
    'BDAY': {
        name: 'Birthday',
        description: 'Birthday Date',
        type: 'date-and-or-time',
        versions: true,
    },
    'CALADRURI': {
        name: 'Calendar Link'
    },

    'VERSION': {
        name: 'vCard Version',
        description: 'vCard File Version',
        type: 'string',
        versions: true,
    },
};

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

    version: '2.1' | '3.0' | '4.0' | string = '3.0';

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

// VC
VCard.Field = {

};