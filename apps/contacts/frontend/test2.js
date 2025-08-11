import ICAL from 'ical.js';
function _splitZone(aValue, isFromIcal) {
      let lastChar = aValue.length - 1;
      let signChar = aValue.length - (isFromIcal ? 5 : 6);
      let sign = aValue[signChar];
      let zone, value;

      if (aValue[lastChar] == 'Z') {
        zone = aValue[lastChar];
        value = aValue.slice(0, Math.max(0, lastChar));
      } else if (aValue.length > 6 && (sign == '-' || sign == '+')) {
        zone = aValue.slice(signChar);
        value = aValue.slice(0, Math.max(0, signChar));
      } else {
        zone = "";
        value = aValue;
      }

      return [zone, value];
    }

  function fromString(aString) {
    // -05:00
    let options = {};
    //TODO: support seconds per rfc5545 ?
    // there are no colons in 
    options.factor = (aString[0] === '+') ? 1 : -1;
    options.hours = strictParseInt(aString.slice(1, 3));
    options.minutes = strictParseInt(aString.slice(4, 6));

    return new UtcOffset(options);
  }
  
  /**
   * Returns a new ICAL.VCardTime instance from a date and/or time string.
   *
   * @param {String} aValue     The string to create from
   * @param {String} aIcalType  The type for this instance, e.g. date-and-or-time
   * @return {VCardTime}        The date/time instance
   */
  function fromDateAndOrTimeString(aValue, aIcalType) {
    function part(v, s, e) {
      return v ? parseInt(v.slice(s, s + e)) : null;
    }
    let parts = aValue.split('T');
    let dt = parts[0], tmz = parts[1];
    let splitzone = tmz ? _splitZone(tmz) : [];
    let zone = splitzone[0], tm = splitzone[1];

    let dtlen = dt ? dt.length : 0;
    let tmlen = tm ? tm.length : 0;

    let hasDashDate = dt && dt[0] == '-' && dt[1] == '-';
    let hasDashTime = tm && tm[0] == '-';

    let o = {
      year: hasDashDate ? null : part(dt, 0, 4),
      month: hasDashDate && (dtlen == 4 || dtlen == 7) ? part(dt, 2, 2) : dtlen == 7 ? part(dt, 5, 2) : dtlen == 10 ? part(dt, 5, 2) : null,
      day: dtlen == 5 ? part(dt, 3, 2) : dtlen == 7 && hasDashDate ? part(dt, 5, 2) : dtlen == 10 ? part(dt, 8, 2) : null,

      hour: hasDashTime ? null : part(tm, 0, 2),
      minute: hasDashTime && tmlen == 3 ? part(tm, 1, 2) : tmlen > 4 ? hasDashTime ? part(tm, 1, 2) : part(tm, 3, 2) : null,
      second: tmlen == 4 ? part(tm, 2, 2) : tmlen == 6 ? part(tm, 4, 2) : tmlen == 8 ? part(tm, 6, 2) : null,
      tzHours: null,
      tzMins: null,
    };

    if (zone == 'Z') {
      o.tzHours = 0;
      o.tzMins = 0;
    } else if (zone && zone[3] == ':') { // WRONG!
      zone = UtcOffset.fromString(zone);

        o.tzFactor = (zone[0] === '+') ? 1 : -1;
        o.tzHours = strictParseInt(zone.slice(1, 3));
        o.tzMins = strictParseInt(zone.slice(4, 6));
    } else {
      o.tzFactor = null;
    }

    return o;

    // return new VCardTime(o, zone, aIcalType);
  }



const tests = [
    '19961022T140000',
    '--1022T1400',
    '---22T14',
    '19850412',
    '1985-04',
    '1985',
    '--0412',
    '---12',
    'T102200',
    'T1022',
    'T10',
    'T-2200',
    'T--00',
    'T102200Z',
    'T102200-0800',
    '19961022T140000',
    '19961022T140000Z',
    '19961022T140000-05',
    '19961022T140000-0500',
    '19961022T141234-0500',
];

tests.map(s => fromDateAndOrTimeString(s)).map((o,i) => console.log(tests[i], o));

// tests
//     .map(s => ICAL.VCardTime.fromDateAndOrTimeString(s, 'date-and-or-time'))
//     .forEach((t, i) => console.log(tests[i],'\n\t', t.toString(),'\n\t', t.toICALString()));
