import { VCProperty } from "./VCProperty";
import {VCDateTime} from "./VCDateTime";

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

export class CustomDateInput extends CustomInput {
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
            const o = VCDateTime.parse(this.property.value);
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
    <input type="date" id="${this.id}" value={this.initialValue()}>
</div>`;
        document.getElementById(this.id).addEventListener('change', this.validate.bind(this));
    }

    loadValue() {
        const e = document.getElementById(this.id) as HTMLInputElement;

        // format YYYY-MM-DD -> YYYYMMDD
        this.property.value = e.value.replaceAll('-','');
    }
}


export class CustomEmailInput extends CustomInput {
   validate(): string | undefined {
       const e = document.getElementById(this.id) as HTMLInputElement;
       if (!/[^\s@]@[^\s@]/.test(e.value.trim())) {
           return 'Invalid email';
       }
   }

   typeOptionsHtml() {
       const selected = this.property.value;
       let ret = '';
       if (selected)
           ret += `<option value="${selected}" selected>${selected}</option>`;

       return ret + `<option value="PERSONAL">Personal</option>
<option value="WORK">Work</option>
<option value="HOME">Home</option>
<option value="OTHER">Other</option>`;
   }

   html(e: HTMLElement) {
       e.innerHTML += `<div class="input-group">
<label for="${this.id}">Email</label>
<input id="${this.id}" type="email" value="${this.property.value || ''}" />
</div><div class="input-group">
<label for="${this.id}-type">Type</label>
<select id="${this.id}-type">${this.typeOptionsHtml()}</select>
</div>`
   }

   loadValue() {
       const v = document.getElementById(this.id) as HTMLInputElement;
       const t = document.getElementById(this.id + '-type') as HTMLInputElement;
       this.property.value = v.value;
       this.property.params.TYPE = t.value;
   }
}

export class CustomTextInput extends CustomInput {

    getValue() {
        const e = document.getElementById(this.id) as HTMLInputElement;
        return e.value;
    }
    override validate(): string | null {
        return this.getValue().length === 0 ? null : "Value cannot be empty";
    }

    override html(e: HTMLElement): void {
        e.innerHTML += `<div class="input-group">
    <label for="${this.id}">${this.property.label()}</label>
    <input type="text" id="${this.id}" value="${this.property.value ||''}" />
</div>`;
    }

    override loadValue(): void {
        this.property.value = this.getValue();
    }
}

export class CustomTextAreaInput extends CustomTextInput {
    override html(e: HTMLElement): void {
        e.innerHTML += `<div class="input-group">
    <label for="${this.id}">${this.property.label()}</label>
    <textarea id="${this.id}" rows="3">${this.property.value ||''}</textarea>
</div>`;
    }
}

export class CustomUriInput extends CustomTextInput {
    override html(e: HTMLElement): void {
        e.innerHTML += `<div class="input-group">
    <label for="${this.id}">${this.property.label()}</label>
    <input type="url" id="${this.id}" value="${this.property.value ||''}" />
</div>`;
    }
}

export class CustomPhoneInput extends CustomInput {
    getValue() {
        const e = document.getElementById(this.id) as HTMLInputElement;
        return e.value;
    }

    override validate(): string | null {
        return this.getValue().length !== 0 ? null : "Phone number cannot be empty";
    }

    typeOptionsHtml() {
        const selected = this.property.value;
        let ret = '';
        if (selected)
            ret += `<option value="${selected}" selected>${selected}</option>`;

        // TODO probably more options
        return ret + `<option value="PERSONAL">Personal</option>
<option value="WORK">Work</option>
<option value="HOME">Home</option>
<option value="OTHER">Other</option>`;
    }

    override html(e: HTMLElement) {
        e.innerHTML += `<div class="input-group">
    <label for="${this.id}">Phone number</label>
    <input id="${this.id}" type="tel" placeholder="+15558675309" value="${this.property.value || ''}" />
</div><div class="input-group">
    <label for="${this.id}-type">Type</label>
    <select id="${this.id}-type">${this.typeOptionsHtml()}</select>
</div>`;
    }

    loadValue() {
        const v = document.getElementById(this.id) as HTMLInputElement;
        const t = document.getElementById(this.id + '-type') as HTMLInputElement;
        this.property.value = v.value;
        this.property.params.TYPE = t.value;
    }
}

// TODO CustomGenderInput
// TODO CustomAddressInput
// TODO CustomGeoInput
// TODO CustomNameInput
// TODO CustomImageInput
// TODO CustomTextOptionsInput
// TODO CustomTimezoneInput
// TODO CustomLangInput