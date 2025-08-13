
import { VCProperty } from "./VCProperty";


export default class VC {
    public static readonly BEGIN_TOKEN = "BEGIN:VCARD";
    public static readonly END_TOKEN = "END:VCARD";

    properties: VCProperty[] = [];

    addProperty(property: VCProperty) {
        this.properties.push(property);
    }

    getDisplayName() {
        let p = this.properties.find(p => p.name === 'FN');
        if (p)
            return p.value;

        p = this.properties.find(p => p.name === 'N');
        if (p)
            return p.nameToString();

        // Either N or FN is required by the vCard spec
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
        if (!p)
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
}