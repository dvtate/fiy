import VC from './VC';
import * as API from './api';

const contactList = document.getElementById("contact-list");
const contactDetails = document.getElementById("contact-details");
const searchInput = document.getElementById("search") as HTMLInputElement;
const sidebar = document.querySelector(".sidebar");
const detailsView = document.querySelector(".details-view");

let uidCounter = 0;

let contacts: VC[];

API.getUserContacts().then(vcf => {
    contacts = VC.parseCards(vcf);

    // Initial render
    const queryParams = new URL(document.location.toString()).searchParams;
    renderContactsList(queryParams.get('q') || "");
    if (queryParams.get('id')) {
        // Show contact by id
        const index = contacts.findIndex(c => c.getId() == queryParams.get('id'));
        showContactDetails(index);
    } else if (queryParams.get('user')) {
        const index = contacts.findIndex(c => c.getFiyUser() == queryParams.get('user'));
        showContactDetails(index);
    } else {
        // Show user profile editable
        // TODO
    }

});

function renderContactsList(filter = "") {
    contactList.innerHTML = "";
    contacts
        .filter(c => c.getDisplayName().toLowerCase().includes(filter.toLowerCase()))
        .forEach((contact, index) => {
            const li = document.createElement("li");
            li.textContent = contact.getDisplayName();
            li.title = contact.getDisplayName();
            li.onclick = () => showContactDetails(index);
            contactList.appendChild(li);
        });
}

function mobileDetailsView(goBack = false) {
    if (window.innerWidth > 768)
        return;

    if (goBack) {
        sidebar.classList.remove("hidden");
        detailsView.classList.remove("active");
    } else {
        sidebar.classList.add("hidden");
        detailsView.classList.add("active");
    }
}

function showContactDetails(index) {

    const contact = contacts[index];
    contactDetails.innerHTML = `
<h2>${contact.getDisplayName()}</h2>
<h3>${contact.getDisplayName()}</h3>
${contact.properties.map(p => p.showHtml()).join('<hr>')}
<hr>
<button title="Download" id="contact-download"><i class="fa fa-download"></i></button>
<button title="Share"><i class="fa fa-share" id="contact-share"></i></button>
<button title="Edit"><i class="fa fa-pencil" id="contact-edit"></i></button>
<button title="Delete"><i class="fa fa-trash" id="contact-delete"></i></button>`;

    document.getElementById('contact-download').addEventListener('click', e => {
        alert("not implemented!");
    });
    document.getElementById('contact-share').addEventListener('click', e => {
        alert("not implemented!");
    });
    document.getElementById('contact-edit').addEventListener('click', e => {

    })

    mobileDetailsView();
}

searchInput.addEventListener("input", () => {
    renderContactsList(searchInput.value);
});

