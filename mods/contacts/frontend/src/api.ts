import VC from "./VC";

export const base_uri = "{{fediy_contacts_base_uri}}";
export const domain = "{{fediy_contacts_domain}}";

const contactsStrTmp = `BEGIN:VCARD
VERSION:2.1
REV:20151216T141119Z
X-GENERATOR:IntSig
X-IS-ANGLE:90
N;CHARSET=utf-8:周;宪
X-IS-INFO:0,0,0,0,0,0,0,0;1,2,13
FN;CHARSET=utf-8:周宪
X-IS-INFO:540,484,583,484,583,615,540,615;0,9,10,13
TEL;CELL;VOICE:18573183138
X-IS-INFO:400,373,423,373,423,619,400,619;6,13
TEL;WORK;VOICE:073188033808
X-IS-INFO:369,338,392,338,392,619,369,619;3,4,5,6,13
EMAIL;PREF;INTERNET:66426222@qq.com
X-IS-INFO:336,296,362,296,362,619,336,619;7,14,13
URL;WORK;CHARSET=utf-8:www.zzyq.cc
X-IS-INFO:304,376,331,376,331,620,304,620;8,13
TITLE;CHARSET=utf-8:研发总监
X-IS-INFO:540,331,565,331,565,441,540,441;9,16,11,10,0,13
ORG;CHARSET=utf-8:北京中周理才信息技术有限公司;
X-IS-INFO:477,106,508,106,508,617,477,617;10,9,11,0,13
ADR;WORK;PREF;CHARSET=utf-8:;;开福区北辰三角洲 D3区1栋3001;长沙市;;410008;中国
X-IS-INFO:273,121,452,121,452,621,273,621;11,13
LABEL;WORK;PREF;CHARSET=utf-8:长沙市开福区北辰三角洲 D3区1栋3001, 410008
X-IS-INFO:273,121,452,121,452,621,273,621;11,13
NOTE;CHARSET=utf-8:中周至尚
X-IS-INFO:316,740,366,740,366,957,316,957;13,10,11,9,0
NOTE;CHARSET=utf-8:ZINGROW
X-IS-INFO:279,739,311,739,311,964,279,964;13,10,11,9,0
GEO:39.95;-75.1667
SOUND;OGG:http://example.com/sound.ogg
END:VCARD

BEGIN:VCARD
VERSION:3.0
N:Gump;Forrest;;Mr.
FN:Forrest Gump
ORG:Bubba Gump Shrimp Co.
TITLE:Shrimp Man
PHOTO;VALUE=URL;TYPE=GIF:http://www.example.com/dir_photos/my_photo.gif
TEL;TYPE=WORK,VOICE:(111) 555-1212
TEL;TYPE=HOME,VOICE:(404) 555-1212
ADR;TYPE=WORK:;;100 Waters Edge;Baytown;LA;30314;United States of America
LABEL;TYPE=WORK:100 Waters Edge\\nBaytown\\, LA 30314\\nUnited States of Ameri
 ca
ADR;TYPE=HOME:;;42 Plantation St.;Baytown;LA;30314;United States of America
LABEL;TYPE=HOME:42 Plantation St.\\nBaytown\\, LA 30314\\nUnited States of Ame
 rica
EMAIL;TYPE=PREF,INTERNET:forrestgump@example.com
IMPP;X-SERVICE-TYPE=Skype;type=WORK;type=pref:skype:neil.mckeown2
item1.ADR;TYPE=WORK:;;100 Waters Edge;Baytown;LA;30314;United States of America
PRODID:-//Apple Inc.//iOS 7.0.4//EN
REV:2008-04-24T19:52:43Z
END:VCARD

BEGIN:VCARD
VERSION:4.0
N:Gump;Forrest;;;
FN:Forrest Gump
ORG:Bubba Gump Shrimp Co.
TITLE:Shrimp Man
PHOTO;MEDIATYPE=image/gif:http://www.example.com/dir_photos/my_photo.gif
TEL;TYPE=work,voice;VALUE=uri:tel:+11115551212
TEL;TYPE=home,voice;VALUE=uri:tel:+14045551212
ADR;TYPE=work;LABEL="100 \\" Waters Edge\\nBaytown, LA 30314\\nUnited States of A
 merica":;;100 Waters Edge;Baytown;LA;30314;United States of America
ADR;TYPE=home;LABEL="42 Plantation St.\\nBaytown, LA 30314\\nUnited States of
 America":;;42 Plantation St.;Baytown;LA;30314;United States of America
EMAIL:forrestgump@example.com
REV:20080424T195243Z
LANG:fr-CA
GEO:geo:39.95,-75.1667
END:VCARD`;

export async function getUserContacts() {
    // Just for now while I work on frontend
    return contactsStrTmp;
    // const r = await fetch(base_uri + '/all');
    // return await r.text();
}

export async function updateContact(contact: VC) {

}

export async function deleteContact(contact: VC) {

}

export async function newContact(contact: VC) {

}