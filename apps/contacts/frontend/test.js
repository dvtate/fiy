
function dateToStr(d) {
    return [
        d.year === null ? '_' : d.year,
        d.month === null ? '_' : d.month,
        d.day === null ? '_' : d.day,
        'T',
        d.hour === null ? '_' : d.hour,
        d.min === null ? '_' : d.min,
        d.sec === null ? '_' : d.sec,
        'Z',
        d.tzHours === null ? '_' : d.tzHours,
        d.tzMins === null ? '_' : d.tzMins,
    ].join(' ');
}


function parseDate(str, ret) {
    /*
      date = year    [month  day]
           / year "-" month
           / "--"     month [day]
           / "--"      "-"   day
    */
    
    if (str.length === 0)
        return;

    // No date, no month, only day
    if (str.startsWith('---')) {
        
        if (str.length < 5)
            return;
        ret.day = parseInt(str.slice(3, 5));
        return;
    }
    
    // Parse year
    if (str.startsWith('--')) {    
        // no year
        ret.year = null;
        str = str.slice(2);
    } else {
        // YYYY
        if (str.length < 4)
            return;
        ret.year = parseInt(str.slice(0, 4));
        str = str.slice(4);
    }
    
    if (str.length === 0)
        return;

    // Parse month 
    if (str[0] == '-') {
        // year - month
        ret.month = parseInt(str.slice(1, 3));
        return;
    }
    ret.month = parseInt(str.slice(0, 2));

    // Parse day
    ret.day = str.length >= 4 ? parseInt(str.slice(2,4)) : null;
}


function parseTime(str, ret) {
    /* no colons
      time = hour [ ":" minute ":" second   [ "." secfrac ] ]
           / hour [     minute    [second]] [zone]
           /  "-"       minute    [second]
           /  "-"         "-"      second

    // not parsed here:
      zone = "Z"
           / ( "+" / "-" ) hour [minute]
    */

   // Get hour
   if (str.length === 0)
       return;
    if (str[0] === '-') {
        ret.hour = null;
        str = str.slice(1);
    } else {
        if (str.length < 2)
            return;
        ret.hour = parseInt(str.slice(0, 2));
        str = str.slice(2);
    }
    
    // Get minute
    if (str.length === 0)
        return;
    if (str[0] === '-') {
        ret.min = null;
        str = str.slice(1);
    } else {
        if (str.length < 2)
            return;
        ret.min = parseInt(str.slice(0, 2));
        str = str.slice(2);
    }

    // Get second
    if (str.length < 2)
        return;
    ret.sec = parseInt(str.slice(0, 2));

    // Not in vcard spec
    // no secfrac
    // if (str[2] !== '.')
    //     return;
    // ret.secFrac = parseInt(str.slice(3));
}

function parseVcardDateTime(str, bareTime = false) {
    // Start with null datetime
    const ret = {
        year: null,
        month: null,
        day: null,
        hour: null,
        min: null,
        sec: null,
        tzHours: null,
        tzMins: null,
    };
    
    const tIndex = str.indexOf('T');
    const dateStr = tIndex === -1 ? str :  str.slice(0, tIndex);
    parseDate(dateStr, ret);
    
    if (tIndex === -1)
        return ret;
    
    let tzIndex = str.lastIndexOf('Z');
    let tzType = 'Z';
    if (tzIndex === -1)
        tzIndex = str.lastIndexOf(tzType = '+');
    if (tzIndex === -1 && str[tIndex + 1] !== '-')
        // note: spec doesn't allow missing hours/minutes when tz specified
        tzIndex = str.lastIndexOf(tzType = '-');
    if (tzIndex === -1 || tzIndex < tIndex)
        tzIndex = undefined;

    const timeStr = str.slice(tIndex + 1, tzIndex);
    parseTime(timeStr, ret);

    // const tzStr = str.slice(tzIndex);
    // console.log({ dateStr, timeStr, tzStr });

    // No timezone
    if (!tzIndex) {
        return ret;
    }

    // UTC
    if (tzType === 'Z') {
        ret.tzHours = 0;
        ret.tzMins = 0;
        return ret;
    }

    // Other timezone
    if (str.length - tzIndex >= 2) {
        ret.tzHours = parseInt(str.slice(tzIndex+1, tzIndex+3));
        if (tzType === '-')
            ret.tzHours = -ret.tzHours;
    }
    if (str.length - tzIndex >= 4)
        ret.tzMins =  parseInt(str.slice(tzIndex+4, tzIndex+6));
    return ret;
}



const tests = [
    ['19961022T140000', '1996 10 22 T 14 0 0 Z _ _' ],
    ['--1022T1400', '_ 10 22 T 14 0 _ Z _ _'],
    ['---22T14', '_ _ 22 T 13 _ _ Z _ _'],
    ['19850412', ],
    ['1985-04', ],
    ['1985', ],
    ['--0412', ],
    ['---12', ],
    ['T102200', ],
    ['T1022', ],
    ['T10', ],
    ['T-2200', ],
    ['T--00', ],
    ['T102200Z', ],
    ['T102200-0800', ],
    ['19961022T140000', ],
    ['19961022T140000Z', ],
    ['19961022T140000-05', ],
    ['19961022T140000-0500', ],
    ['19961022T141234-0500', ],
];

tests.forEach(([s, r]) => {
    const ret = parseVcardDateTime(s);
    console.log(s, dateToStr(ret), ret);
});
