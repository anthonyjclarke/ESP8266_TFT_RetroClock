/*
 * timezones.h - Global Timezone Definitions
 * 
 * Contains POSIX timezone strings for 88 global cities
 * Used for NTP time synchronization with automatic DST handling
 * 
 * To add a new timezone:
 * 1. Find the POSIX TZ string for your location
 * 2. Add entry to timezones[] array: {"City, Country", "TZ_STRING"}
 * 3. Increment will be automatic via sizeof calculation
 * 
 * POSIX TZ String Format:
 * STDoffset[DST[offset],start[/time],end[/time]]
 * 
 * Example: "AEST-10AEDT,M10.1.0,M4.1.0/3"
 * - AEST = Standard time name
 * - -10 = Offset from UTC (note: sign is reversed)
 * - AEDT = Daylight saving time name
 * - M10.1.0 = DST starts month 10, week 1, Sunday
 * - M4.1.0/3 = DST ends month 4, week 1, Sunday at 3am
 */

#ifndef TIMEZONES_H
#define TIMEZONES_H

// ======================== TIMEZONE CONFIGURATION ========================
struct TimezoneInfo {
  const char* name;
  const char* tzString;
};

// Expanded timezone array with 88 global timezones
// Organized by region: Australia, Americas, Europe, Asia, Pacific, Africa, Middle East
const TimezoneInfo timezones[] = {
  // ==================== AUSTRALIA & OCEANIA ====================
  {"Sydney, Australia", "AEST-10AEDT,M10.1.0,M4.1.0/3"},
  {"Melbourne, Australia", "AEST-10AEDT,M10.1.0,M4.1.0/3"},
  {"Brisbane, Australia", "AEST-10"},
  {"Adelaide, Australia", "ACST-9:30ACDT,M10.1.0,M4.1.0/3"},
  {"Perth, Australia", "AWST-8"},
  {"Darwin, Australia", "ACST-9:30"},
  {"Hobart, Australia", "AEST-10AEDT,M10.1.0,M4.1.0/3"},
  {"Auckland, New Zealand", "NZST-12NZDT,M9.5.0,M4.1.0/3"},
  {"Wellington, New Zealand", "NZST-12NZDT,M9.5.0,M4.1.0/3"},
  {"Fiji", "FJT-12FJST,M11.1.0,M1.3.0/3"},
  {"Port Moresby, Papua New Guinea", "PGT-10"},
  {"Noumea, New Caledonia", "NCT-11"},
  
  // ==================== NORTH AMERICA ====================
  {"New York, USA", "EST5EDT,M3.2.0,M11.1.0"},
  {"Los Angeles, USA", "PST8PDT,M3.2.0,M11.1.0"},
  {"Chicago, USA", "CST6CDT,M3.2.0,M11.1.0"},
  {"Denver, USA", "MST7MDT,M3.2.0,M11.1.0"},
  {"Phoenix, USA", "MST7"},
  {"Anchorage, USA", "AKST9AKDT,M3.2.0,M11.1.0"},
  {"Honolulu, USA", "HST10"},
  {"Toronto, Canada", "EST5EDT,M3.2.0,M11.1.0"},
  {"Vancouver, Canada", "PST8PDT,M3.2.0,M11.1.0"},
  {"Montreal, Canada", "EST5EDT,M3.2.0,M11.1.0"},
  {"Mexico City, Mexico", "CST6CDT,M4.1.0,M10.5.0"},
  
  // ==================== SOUTH AMERICA ====================
  {"Sao Paulo, Brazil", "BRT3BRST,M10.3.0/0,M2.3.0/0"},
  {"Buenos Aires, Argentina", "ART3"},
  {"Santiago, Chile", "CLT4CLST,M8.2.6/24,M5.2.6/24"},
  {"Lima, Peru", "PET5"},
  {"Bogota, Colombia", "COT5"},
  {"Caracas, Venezuela", "VET4:30"},
  
  // ==================== WESTERN EUROPE ====================
  {"London, UK", "GMT0BST,M3.5.0/1,M10.5.0"},
  {"Paris, France", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Berlin, Germany", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Rome, Italy", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Madrid, Spain", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Amsterdam, Netherlands", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Brussels, Belgium", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Vienna, Austria", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Zurich, Switzerland", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Lisbon, Portugal", "WET0WEST,M3.5.0/1,M10.5.0"},
  {"Dublin, Ireland", "IST-1GMT0,M10.5.0,M3.5.0/1"},
  {"Reykjavik, Iceland", "GMT0"},
  
  // ==================== NORTHERN EUROPE ====================
  {"Stockholm, Sweden", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Oslo, Norway", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Copenhagen, Denmark", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Helsinki, Finland", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
  
  // ==================== CENTRAL & EASTERN EUROPE ====================
  {"Prague, Czech Republic", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Warsaw, Poland", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Budapest, Hungary", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Athens, Greece", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
  {"Bucharest, Romania", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
  {"Sofia, Bulgaria", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
  {"Kiev, Ukraine", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
  {"Moscow, Russia", "MSK-3"},
  {"Minsk, Belarus", "MSK-3"},
  
  // ==================== MIDDLE EAST ====================
  {"Dubai, UAE", "GST-4"},
  {"Tel Aviv, Israel", "IST-2IDT,M3.4.4/26,M10.5.0"},
  {"Istanbul, Turkey", "TRT-3"},
  {"Riyadh, Saudi Arabia", "AST-3"},
  {"Tehran, Iran", "IRST-3:30IRDT,J79/24,J263/24"},
  
  // ==================== SOUTH ASIA ====================
  {"Mumbai, India", "IST-5:30"},
  {"Colombo, Sri Lanka", "IST-5:30"},
  {"Kathmandu, Nepal", "NPT-5:45"},
  {"Dhaka, Bangladesh", "BST-6"},
  {"Karachi, Pakistan", "PKT-5"},
  {"Kabul, Afghanistan", "AFT-4:30"},
  {"Thimphu, Bhutan", "BTT-6"},
  
  // ==================== SOUTHEAST ASIA ====================
  {"Bangkok, Thailand", "ICT-7"},
  {"Singapore", "SGT-8"},
  {"Jakarta, Indonesia", "WIB-7"},
  {"Manila, Philippines", "PHT-8"},
  {"Kuala Lumpur, Malaysia", "MYT-8"},
  {"Ho Chi Minh, Vietnam", "ICT-7"},
  {"Yangon, Myanmar", "MMT-6:30"},
  
  // ==================== EAST ASIA ====================
  {"Hong Kong", "HKT-8"},
  {"Shanghai, China", "CST-8"},
  {"Taipei, Taiwan", "CST-8"},
  {"Tokyo, Japan", "JST-9"},
  {"Seoul, South Korea", "KST-9"},
  {"Ulaanbaatar, Mongolia", "ULAT-8"},
  
  // ==================== CENTRAL ASIA ====================
  {"Tashkent, Uzbekistan", "UZT-5"},
  {"Almaty, Kazakhstan", "ALMT-6"},
  {"Bishkek, Kyrgyzstan", "KGT-6"},
  
  // ==================== CAUCASUS ====================
  {"Yerevan, Armenia", "AMT-4"},
  {"Tbilisi, Georgia", "GET-4"},
  {"Baku, Azerbaijan", "AZT-4"},
  
  // ==================== AFRICA ====================
  {"Johannesburg, South Africa", "SAST-2"},
  {"Cairo, Egypt", "EET-2"},
  {"Lagos, Nigeria", "WAT-1"},
  {"Nairobi, Kenya", "EAT-3"}
};

// Calculate number of timezones automatically
const int numTimezones = sizeof(timezones) / sizeof(timezones[0]);

#endif // TIMEZONES_H
