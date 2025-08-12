/**
 * vCard DateTime
 */
export class VCDT {
    year?: string;
    month?: string;
    day?: string;
    hour?: string;
    min?: string;
    sec?: string;
    zone?: string;

    static parse(str: string, bareTime = false) {
        // Start with null datetime
        const ret = new VCDT();

        // TODO replace this with something more performant
        const dateRegex =
            /(?:^(?<year>[0-9]{4})(?<month>[01][0-9])(?<day>[0-3][0-9])$)/.source
            + /|(?:^(?<year>[0-9]{4})-(?<month>[01][0-9])-(?<day>[0-3][0-9])$)/.source
            + /|(?:^--(?<month>[01][0-9])(?<day>[0-3][0-9])$)/.source
            + /|(?:^---(?<day>[0-3][0-9])$)/.source
            + /|(?:^(?<year>[0-9]{4})-(?<month>[01][0-9])$)/.source
            + /|(?:^(?<year>[0-9]{4})(?<month>[01][0-9])$)/.source
            + /|(?:^(?<year>[0-9]{4})$)/.source;
        const timeRegex =
            /(?:^(?<hour>[0-2][0-9])(?<min>[0-6][0-9])(?<sec>[0-6][0-9])(?<zone>Z|(?:[+\-][0-2][0-9](?:[0-2][0-9])?))?$)/.source
            + /|(?:^(?<hour>[0-2][0-9]):(?<min>[0-6][0-9]):(?<sec>[0-6][0-9])(?<zone>Z|(?:[+\-][0-2][0-9](?::[0-2][0-9])?))?$)/.source
            + /|(?:^(?<hour>[0-2][0-9])(?<min>[0-6][0-9])(?<sec>[0-6][0-9])$)/.source
            + /|(?:-(?<min>[0-6][0-9])(?<sec>[0-6][0-9])$)/.source
            + /|(?:--(?<sec>[0-6][0-9])$)/.source
            + /|(?:^(?<hour>[0-2][0-9])(?<min>[0-6][0-9])$)/.source
            + /|(?:^(?<hour>[0-2][0-9])$)/.source;

        if (bareTime) {
            const m = str.match(timeRegex);
            if (!m)
                return null;
            ret.hour = m.groups.hour;
            ret.min = m.groups.min;
            ret.sec = m.groups.sec;
            ret.zone = m.groups.zone;
            return ret;
        }

        const [dateStr, timeStr] = str.split('T');

        if (dateStr) {
            const m = dateStr.match(dateRegex);
            if (!m) {
                console.log('invalid date: ', dateStr);
                // return null;
            }
            ret.year = m.groups.year;
            ret.month = m.groups.month;
            ret.day = m.groups.day;
        }
        if (timeStr) {
            const m = timeStr.match(timeRegex);
            if (!m) {
                console.log('invalid time2', timeStr);
                // return null;
            }
            ret.hour = m.groups.hour;
            ret.min = m.groups.min;
            ret.sec = m.groups.sec;
            ret.zone = m.groups.zone;
        }

        return ret;
    }

    toUserString() {
        const monthNames = [
            '',
            'January',
            'February',
            'March',
            'April',
            'May',
            'June',
            'August',
            'September',
            'October',
            'November',
            'December'
        ];

        let ret = '';
        if (!this.year)
            ret += "Every ";
        if (this.month)
            ret += monthNames[parseInt(this.month)];
        if (this.day) {
            ret += " ";
            const iday = parseInt(this.day);
            ret += iday;
            const ord = iday % 10 === 1
                ? 'st'
                : iday % 10 === 2
                    ? 'nd'
                    : 'th';
            ret += ord;
        }

        // Add time
        if (this.hour || this.min || this.sec) {
            ret += " at ";
        }
        ret += this.hour || '**';
        ret += ':';
        ret += this.min || '**';
        if (this.sec)
            ret += ':' + this.sec;

        // Add zone info
        if (this.zone) {
            ret += 'UTC';
            if (this.zone !== 'Z')
                ret += this.zone;
        }
        return ret;
    }

    toVCString() {
        let ret = '';
        if (this.year || this.month || this.day) {
            ret += this.year || '--';
            ret += this.month || (this.day ? '-' : '');
            if (this.day)
                ret += this.day;
        }
        if (this.hour || this.min || this.sec) {
            ret += 'T';
            ret += this.hour || '-';
            ret += this.min || (this.sec ? '-' : '');
            if (this.sec)
                ret += this.sec;
        }
        if (this.zone)
            ret += this.zone;

        return ret;
    }

};