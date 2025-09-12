import VC from "./VC";

export const base_uri = "{{fediy_contacts_base_uri}}";
export const domain = "{{fediy_contacts_domain}}";

export async function getUserContacts() {
    // Just for now while I work on frontend
    const r = await fetch(base_uri + '/all');
    return await r.text();
}

export async function updateContact(contact: VC) {
    const r = await fetch(base_uri + '/save', {
        method: 'POST',
        body: contact.vCardString()
    });
    return r.text();
}

export async function deleteContact(contact: VC) {
    const r = await fetch(base_uri + '/delete/' + contact.getId(), {
        method: 'DELETE',
    });
    return r.text();
}

export async function newContact(contact: VC) {

}