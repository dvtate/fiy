import ICAL from "ical.js";


const capitalized = (s: string) => s.charAt(0).toLocaleUpperCase() + s.slice(1);

type VCValueType = 'boolean' | 'DATE' | 'DATE-AND-OR-TIME' | 'DATE-TIME' | 'FLOAT' | 'INTEGER' | 'LANGUAGE-TAG' | 'TEXT' | 'TIME' | 'TIMESTAMP' | 'URI' | 'UTC-OFFSET' | string;

/// Params for a property
interface VCParams {
    // +-----------+-----------+------------------------+
    // | Namespace | Parameter | Reference              |
    // +-----------+-----------+------------------------+
    // |           | LANGUAGE  | RFC 6350, Section 5.1  |
    // |           | VALUE     | RFC 6350, Section 5.2  |
    // |           | PREF      | RFC 6350, Section 5.3  |
    // |           | ALTID     | RFC 6350, Section 5.4  |
    // |           | PID       | RFC 6350, Section 5.5  |
    // |           | TYPE      | RFC 6350, Section 5.6  |
    // |           | MEDIATYPE | RFC 6350, Section 5.7  |
    // |           | CALSCALE  | RFC 6350, Section 5.8  |
    // |           | SORT-AS   | RFC 6350, Section 5.9  |
    // |           | GEO       | RFC 6350, Section 5.10 |
    // |           | TZ        | RFC 6350, Section 5.11 |
    // +-----------+-----------+------------------------+


    /// Language for value
    LANGUAGE?: string;

    /// Value type for the field
    // +------------------+-------------------------+
    // | Value Data Type  | Reference               |
    // +------------------+-------------------------+
    // | BOOLEAN          | RFC 6350, Section 4.4   |
    // | DATE             | RFC 6350, Section 4.3.1 |
    // | DATE-AND-OR-TIME | RFC 6350, Section 4.3.4 |
    // | DATE-TIME        | RFC 6350, Section 4.3.3 |
    // | FLOAT            | RFC 6350, Section 4.6   |
    // | INTEGER          | RFC 6350, Section 4.5   |
    // | LANGUAGE-TAG     | RFC 6350, Section 4.8   |
    // | TEXT             | RFC 6350, Section 4.1   |
    // | TIME             | RFC 6350, Section 4.3.2 |
    // | TIMESTAMP        | RFC 6350, Section 4.3.5 |
    // | URI              | RFC 6350, Section 4.2   |
    // | UTC-OFFSET       | RFC 6350, Section 4.7   |
    // +------------------+-------------------------+
    VALUE?: VCValueType;

    /// Preference level 0-100
    PREF?: number;

    /// Used for language overloading
    ALTID?: number;

    /// Used to identify specific properties among duplicates
    PID?: string[];

    /// Various uses (ie - home, work, fax, etc.)
    TYPE?: string;

    /// Content-Type
    MEDIATYPE?: string;

    CALSCALE?: string;

};

const vCardFieldNames = [
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

const vCardProperties = {
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
    'CALURI': {
        name: 'Calendar Link',
        description: 'Link to calender for sending scheduling requests',
        type: 'uri',
        versions: ['4.0'],
    },
    'CATEGORIES': {
        name: 'Categories',
        description: 'List of descriptive tags',
        type: 'text', // comma separated tags
        versions: ['3.0', '4.0'],
    },
    'CLASS': {
        name: 'Class',
        description: 'Describes sensitivity of this information',
        type: 'text',
        versions: ['3.0'],
    },
    'CLIENTPIDMAP': {
        name: 'Revision History',
        description: 'Not supported',
        internal: true,
        type: 'text',
        versions: false,
    },
    'EMAIL': {
        name: 'Email',
        description: 'Email address',
        type: 'text', // does not include mailto:
        versions: true
    },
    'FBURL': {
        name: 'Free-Busy URL',
        description: 'URL that shows when person is "free" or "busy"',
        type: 'uri',
        versions: ['4.0'],   
    },
    'FN': {
        name: 'Name',
        description: 'Full name, as it should be formatted',
        type: 'text',
        versions: true,
    },
    'GENDER': {
        name: 'Gender',
        description: 'Describes the person\s gender',
        type: 'sex',
        versions: ['4.0'],
    },
    'GEO': {
        name: 'Geographic Location',
        description: 'Related latitude and longitude',
        type: 'geo', // 4.0: geo:lat,lon || 2.1,3.0: geo:lat;lon
        versions: ['2.1','3.0','4.0'],
    },
    'IMPP': {
        name: 'Instant messenger handle',
        description: 'Instant messneger handle',
        type: 'uri',
        versions: ['3.0', '4.0'],
    },
    'KEY': {
        name: 'Public key',
        description: 'Associated cryptographic key for encryption',
        type: 'key',
        versions: ['2.1','3.0','4.0'],
    },
    'KIND': {
        name: 'Entity Kind',
        description: 'Type of entity that this card refers to',
        type: 'text', // allowed: 'application', 'individual', 'group', 'location' or 'organization'; 'x-*'
        versions: ['4.0'],
    },
    'LABEL': {
        name: 'Shipping Address',
        description: 'Text that should go on a mailing label',
        type: 'adr', // same as ADR property
        versions: ['2.1', '3.0'],
        // version 4.0: ADR;LABEL: ...
    },
    'LANG': {
        name: 'Language',
        description: 'Language that a person speaks',
        type: 'lang', // Language code: en-US, fr-CA
        versions: true,
    },
    'LOGO': {
        name: 'Logo',
        description: 'Logo for associated organization',
        type: 'img', // either encoded image or url
        versions: ['2.1','3.0','4.0'],
    },
    'MAILER': {
        name: 'Email App',
        description: 'Email App Used',
        type: 'text',
        versions: ['2.1','3.0'],
    },
    'MEMBER': {
        name: 'Member of this Group',
        description: 'Defines a member of the group that this card represents',
        type: 'uri',
        versions: ['4.0'],
    },
    'N': {
        name: 'Structured Name',
        description: 'Structured representation of the person\'s name',
        type: 'name', // name
        versions: true,
    },
    'NICKNAME': {
        name: 'Nickname',
        description: 'More descriptive/familiar names',
        type: 'text', // comma separated list
        versions: true,
    },
    'NOTE': {
        name: 'Notes',
        description: 'Supplimental information',
        type: 'text',
        versions: true,
    },
    'ORG': {
        name: 'Organization',
        description: 'Units of associated organization',
        type: 'text',
        versions: true,
    },
    'PHOTO': {
        name: 'Photo',
        description: 'Photo of individual/profile picture',
        type: 'img',
        versions: true,
    },
    'PRODID': {
        name: 'Created by',
        description: 'Product that created this card',
        type: 'text',
        internal: true,
        versions: ['3.0','4.0'],
    },
    'PROFILE': {
        name: 'File type',
        description: 'vcard',
        type: 'text', // vcard
        internal: true,
        versions: ['3.0'],
    },
    'RELATED': {
        name: 'Related',
        description: 'Related entity',
        type: 'text',
        versions: ['4.0'],
    },
    'REV': {
        // logic for this should be on frontend...
        name: 'Updated',
        description: 'Last time this contact was edited',
        type: 'timestamp',
        versions: true,
    },
    'ROLE': {
        name: 'Role',
        description: 'Role, Occupation or Business category',
        type: 'text',
        versions: true,
    },
    'SORT-STRING': {
        name: 'Sort By',
        description: 'Sort',
        type: 'text',
        versions: ['3.0'],
    },

    // more from wikipedia



    'VERSION': {
        name: 'vCard Version',
        description: 'vCard File Version',
        type: 'string',
        versions: true,
    },
};

interface VCProp {
    showHtml(): string,
    editHtml(): string,
    toVcard(): string,
    validate(): boolean,
}



export class VCProperty {

    underlyingValue: any = null;

    /**
     * @param name vcard property name
     * @param params vcard parameters
     * @param value raw vcard value
     */
    constructor(
        public name: string,
        public params: VCParams = {},
        public value: string = "",
    ) {
    }

    ////
    // vCard methods
    ////

    paramsString() {
        return Object.entries(this.params).map(([p, v]) => {
            switch (typeof v) {
                case 'string':
                    return `${p}=${v}`;
                case 'object':
                    return `${p}=${v.join(',')}`;
                default:
                    return p;
            }
        }).join(';');
    }

    toLine(): string {
        return `${this.name}${
            this.paramsString()
        }:${
            this.value
        }\r\n`;
    }

    ////
    // Value methods
    ////


    fromJSDate(d: Date) {
        this.fromVCDate(
            d.getUTCDate().toString(),
            d.getUTCMonth().toString(),
            d.getUTCFullYear().toString()
        );
    }

    fromVCDate(day?: string, month?: string, year?: string) {
        if (year && year.length < 4)
            year = '000'.slice(year.length - 4);
        if (year && year.length > 4) {
            console.error('year > 10000');
            year = '+' + year; // ISO-8601 spec but this will probably not work anywhere lol
        }
        let ret = year || '--';
        if (month && month.length == 1)
            ret += '0';
        ret += month || '-';
        if (!month)
            day = undefined;
        if (day && day.length == 1)
            ret += '0';
        ret += day || '';
        this.value = ret;
    }

    toDateObject() {
        return {};
    }

    toDateString() {
        const o = this.toDateObject();

    }

    /**
     * Gender, this documentation is what the spec says, I'm going to prompt something different though.
     * @param sex A single letter representing biological sex.  M stands for "male", F stands
     *    for "female", O stands for "other", N stands for "none or not
     *    applicable", U stands for "unknown".
     * @param identity Gender identity component: free-form text
     */
    fromGender(sex: "" | "M" | "F" | "O" | "N" | "U" = "", identity?: string) {
        this.value = sex;
        if (identity) 
            this.value += ';' + identity;
    }

    // Family Name, Given Name, Additional/Middle Names, Honorific Prefixes, and Honorific Suffixes
    fromName(
        surname: string,
        givenName: string,
        middleNames: string[] = [],
        prefixes: string[] = [],
        suffixes: string[] = [],
    ) {
        this.value = `${surname};${
            givenName};${
                middleNames.join(',')
            };${
                prefixes.join(',')
            };${
                suffixes.join(',')
            }`;
    }

    fromCoords(lat: number, lng: number, version) {
        if (version === '2.1' || version === '3.0')
            this.value = `${lat},${lng}`;
        else
            this.value = `geo:${lat},${lng}`;
    }

    ////
    // UI methods
    ////

    getLabel() {
        return vCardProperties[this.name].name;        
    }

    showHtml() {
        return `<div class="input-group">
        <label for="${this.id}">${}</label>
            <input type="date" id="${this.id}" value={this.initialValue}></input>
        </div>`;
    }

    editHtml() {

    }

    getInput(id: string): string {
        
    }

    getDisplay(): string {

    }

    static VCParam
}