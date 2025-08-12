import { VCProperty } from "./VCProperty";
import {VCDT} from "./VCDT";

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

export abstract class CustomInput {

    protected static _uuid = 0;
    static uid(): string {
        return 'pg-input' + ++CustomInput._uuid;
    }

    protected constructor(
        public property: VCProperty,
        public id: string = CustomInput.uid(),
    ) {}

    /**
     * Validate user input
     * @return erorr message if invalid
     */
    abstract validate(): string | null;

    /**
     * Add input html to element
     * @param e
     */
    abstract html(e: HTMLElement): void;

    /**
     * Update value of relevant VCProperty
     */
    abstract loadValue(): void;
}

export class CustomDateInput extends CustomInput{
    constructor(property: VCProperty) {
        super(property);
    }

    validate(): string | null {
        // const e = document.getElementById(this.id) as HTMLInputElement;
        // const v = e.value;
        return null;
    }

    initialValue() {
        if (this.property.value) {
            const o = VCDT.parse(this.property.value);
            const today = new Date().toISOString().slice(0,10);
            if (!o.year)
                o.year = today.slice(0,4);
            if (!o.month)
                o.month = today.slice(5, 7);
            if (!o.day)
                o.day = today.slice(8, 10);
            return `${o.year}-${o.month}-${o.day}`;
        }
    }

    html(e: HTMLElement) {
        e.innerHTML += `<div class="input-group">
    <label for="${this.id}">${this.property.label()}</label>
    <input type="date" id="${this.id}" value={this.initialValue()}></input>
</div>`;
        document.getElementById(this.id).addEventListener('change', this.validate.bind(this));
    }

    loadValue() {
        const e = document.getElementById(this.id) as HTMLInputElement;

        // format YYYY-MM-DD -> YYYYMMDD
        this.property.value = e.value.replaceAll('-','');
    }
}

// TODO
// CustomImageInput
// CustomGenderInput
// CustomPhoneInput
// CustomTextInput
// CustomTextOptionsInput
// CustomTimezoneInput
// CustomLangInput
// CustomUriInput
// CustomAddressInput
// CustomNameInput
// CustomGeoInput