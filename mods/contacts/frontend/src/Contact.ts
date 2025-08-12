
import { VCProperty } from "./VCProperty";


export default class VCard {
    public static readonly BEGIN_TOKEN = "BEGIN:VCARD";
    public static readonly END_TOKEN = "END:VCARD";

    properties: VCProperty[] = [];

    addProperty(property: VCProperty) {
        this.properties.push(property);
    }

    static parseCard(vc: string) {
        // Not a vcard
        if (!vc.startsWith('BEGIN:VCARD'))
            return null;

        const ret = new VCard();
        const props = vc.split(/\r\n|\n\S/);
        props.forEach(p => {
            const vcp = VCProperty.fromLine(p);
            if (vcp != null)
                ret.properties.push(vcp);
        });

        return ret;
    }

    static parseCards(vcf: string) {
        return vcf.split('END:VCARD')
            .map(s => VCard.parseCard(s.trim()));
    }

}