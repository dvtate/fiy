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

}

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