
import { VCProperty } from "./VCProperty";


export default class VC {
    public static readonly BEGIN_TOKEN = "BEGIN:VCARD";
    public static readonly END_TOKEN = "END:VCARD";

    properties: VCProperty[] = [];

    addProperty(property: VCProperty) {
        this.properties.push(property);
    }

    private displayNameCache: string;

    getDisplayName() {
        if (this.displayNameCache)
            return this.displayNameCache;

        let p = this.properties.find(p => p.name === 'FN');
        if (p)
            return this.displayNameCache = p.value;

        p = this.properties.find(p => p.name === 'N');
        if (p)
            return this.displayNameCache = p.nameToString();

        // Either N or FN is required by the vCard spec
        // So this card is invalid
        return "invalid";
    }

    getId() {
        // Looks like "SOURCE:<protocol><domain>/contacts/id/<contact id>"
        let p = this.properties.find(p => p.name == 'SOURCE');
        if (!p)
            return '-1';
        return p.value.split('/').pop();
    }

    getFiyUser() {
        let p = this.properties.find(p => p.name == 'UID');
        if (!p || !p.value.includes('@'))
            return '';
        return p.value;
    }

    static parseCard(vc: string) {
        // Not a vcard
        vc = vc.trim();
        if (!vc.startsWith('BEGIN:VCARD'))
            return null;

        const ret = new VC();
        // const props = vc.split(/\r\n|\n\S/);
        const props = vc
            // replace escaped new lines
            .replace(/\n\s{1}/g, '')
            // split if a character is directly after a newline
            .split(/\r\n(?=\S)|\r(?=\S)|\n(?=\S)/);
        props.forEach(p => {
            const vcp = VCProperty.fromLine(p);
            if (vcp != null)
                ret.properties.push(vcp);
        });

        return ret;
    }

    static parseCards(vcf: string): VC[] {
        return vcf.split('END:VCARD')
            .slice(0, -1)
            .map(s => VC.parseCard(s.trim()))
            .filter(vc => !!vc);
    }

    /**
     * Convert to vCard v4.0 representation
     * Remove all properties that get set by the server
     */
    convertInternal() {
        // TODO this logic should also be on the backend

        // Remove some properties
        this.properties = this.properties.filter(p =>
            ![
                // Obsolete/pointless
                'AGENT',
                'CLASS',
                'MAILER',
                'PROFILE',
                'SORT-STRING',

                // Not supported
                'CLIENTPIDMAP',

                // Internal only
                'UID',
            ].includes(p.name)
        );

        // Convert LABEL into ADR;LABEL:
        this.properties
            .filter(p => p.name === 'LABEL')
            .forEach(p => {
                p.name = 'ADR';
                if (p.params.TYPE)
                    p.params.TYPE += ',LABEL';
                else
                    p.params.TYPE = 'LABEL';
            });

        const version = this.properties.find(p => p.name === 'VERSION');
        let oldVersion = '4.0';
        if (!version) {
            this.properties.push(new VCProperty('VERSION', {}, '4.0'));
        } else {
            oldVersion = version.value;
            version.value = '4.0';
        }

        // If it's really old, only include known properties
        // if (!['4.0', '3.0'].includes(oldVersion))
            this.properties = this.properties.filter(p => VCProperty.known[p.name]);

        return this;
    }

    vCardString() {
        const version = this.properties.find(p => p.name === 'VERSION');

        let ret = 'BEGIN:VCARD\r\n';
        ret += version.toLine();
        ret += this.properties
            .map(p => p.name === 'VERSION' ? '' : p.toLine())
            .join('\r\n');
        ret += '\r\nEND:VCARD';
        return ret;
    }

    /**
     * Returns the user's username if this card is a profile card, otherwise gives false
     */
    isProfileCard() {
        // For the user's profile card, the server sets a special property:
        //      X-FEDIY-PROFILE: username
        const p = this.properties.find(p => p.name === 'X-FEDIY-PROFILE');
        if (!p)
            return false;
        return p.value;
    }

    /**
     * Gives a representation of the vCard property groups for this card
     *
     * @notes I don't want to support contact groups
     *      but apple,google,etc. abuse them
     */
    propertiesGrouped(): { [group: string] : VCProperty | VCProperty[] } {
        /*
        {
            0: property,
            'group': [ property, property ],
            3: property,
            ...
        }
         */
        let ret = {};
        for (let i = 0; i < this.properties.length; i++) {
            if (this.properties[i].name.includes('.')) {
                const group = this.properties[i].name.split('.')[0];
                if (!ret[group])
                    ret[group] = [];
                ret[group].push(this.properties[i]);
            } else {
                ret[i] = this.properties[i];
            }
        }
        return ret;
    }

    showHtml() {
        const dn = this.isProfileCard()
            ? 'Your Profile'
            : this.getDisplayName();
        const fiyUser = this.getFiyUser();
        const propsHtml = Object.entries(this.propertiesGrouped())
            .map(([k, v]) =>
                v instanceof VCProperty
                    ? v.showHtml()
                    : `<fieldset><legend>${k}</legend>${
                        v.map(v => v.showHtml()).join('<hr/>')
                        }</fieldset>`
            ).join('<hr/>');

        return `<h2>${dn}</h2>${
            fiyUser ? `<h3>${fiyUser}</h3>` : ''
        }${
            propsHtml
        }`;
    }
}