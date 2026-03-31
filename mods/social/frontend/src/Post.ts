
export default class Post {
    constructor(
        public id: number,
        public domain: string,
        public user: string,
        public rawContent: string,
        public createdTs: number,
        public likesCount: number,
        public commentsCount: number,
        public replyToPostId: number,
        public replyToPostDomain: string,
    ) {}

    public dateString() {
        new Date()
    }

}
