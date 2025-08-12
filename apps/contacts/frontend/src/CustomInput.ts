import ICAL from "ical.js";
import { VCProperty } from "./VCProperty";

/*

+------------------+-------------------------+
| Value Data Type  | Reference               |
+------------------+-------------------------+
| BOOLEAN          | RFC 6350, Section 4.4   |
| DATE             | RFC 6350, Section 4.3.1 |
| DATE-AND-OR-TIME | RFC 6350, Section 4.3.4 |
| DATE-TIME        | RFC 6350, Section 4.3.3 |
| FLOAT            | RFC 6350, Section 4.6   |
| INTEGER          | RFC 6350, Section 4.5   |
| LANGUAGE-TAG     | RFC 6350, Section 4.8   |
| TEXT             | RFC 6350, Section 4.1   |
| TIME             | RFC 6350, Section 4.3.2 |
| TIMESTAMP        | RFC 6350, Section 4.3.5 |
| URI              | RFC 6350, Section 4.2   |
| UTC-OFFSET       | RFC 6350, Section 4.7   |
+------------------+-------------------------+
*/

class CustomInputValue {
    constructor(
        public parameters: string[],
        public value: string
    ) {
    }
}

export abstract class CustomInput {

    protected static _uuid = 0;
    static uid(): string {
        return 'pg-input' + ++CustomInput._uuid;
    }

    protected constructor(
        public property: VCProperty,
        public id: string = CustomInput.uid(),
    ) {}

    abstract validate(): string | null;

    abstract html(): string;

    abstract value(): CustomInputValue;
}

export abstract class CustomDateInput extends CustomInput{
    constructor(property: VCProperty) {
        super(property);
    }

    validate(): string | null {
        const e = document.getElementById(this.id) as HTMLInputElement;
        const v = e.value;
        return null;
    }

    initialValue() {
        return ICAL.VCardTime.fromDateAndOrTimeString(this.property.value, this.property.params.VALUE || 'date-and-or-type');
    }

    addHtml(e) {
        const html = `<div class="input-group">
        <label for="${this.id}">${}</label>
    <input type="date" id="${this.id}" value={this.initialValue}></input>
</div>`;
    }
}
