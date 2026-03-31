

export const base_uri = "{{fiy_social_base_uri}}";
export const domain = "{{fiy_social_domain}}";

export async function getPost(id: number, domain: string = "") {
    // Just for now while I work on frontend
    const r = await fetch(base_uri + '/all');
    if (r.status === 404)
    return await r.text();
}