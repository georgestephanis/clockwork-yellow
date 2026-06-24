import math
from datetime import datetime

# Python implementation of the Meeus astronomical algorithms in solar.cpp
# to verify seasonal declination and subsolar longitude transitions.

def get_julian_date(year, month, day, hour, minute, second):
    if month <= 2:
        year -= 1
        month += 12
    A = math.floor(year / 100.0)
    B = 2.0 - A + math.floor(A / 4.0)
    JD = math.floor(365.25 * (year + 4716)) + math.floor(30.6001 * (month + 1)) + day + B - 1524.5
    day_fraction = (hour + minute / 60.0 + second / 3600.0) / 24.0
    return JD + day_fraction

def rad(deg):
    return deg * math.pi / 180.0

def deg(rad):
    return rad * 180.0 / math.pi

def calculate_solar_position(year, month, day, hour, minute, second):
    # 1. Julian Date
    jd = get_julian_date(year, month, day, hour, minute, second)
    utc_hour_decimal = hour + minute / 60.0 + second / 3600.0

    # 2. Julian centuries since J2000.0
    T = (jd - 2451545.0) / 36525.0

    # 3. Geometric Mean Longitude of the Sun
    L0 = 280.46646 + T * (36000.76983 + T * 0.0003032)
    L0 = L0 % 360.0

    # 4. Geometric Mean Anomaly of the Sun
    M = 357.52911 + T * (35999.05029 - T * 0.0001537)

    # 5. Eccentricity of Earth's orbit
    e = 0.016708634 - T * (0.000042037 + T * 0.0000001267)

    # 6. Equation of the Center
    C = math.sin(rad(M)) * (1.914602 - T * (0.004817 + T * 0.000014)) \
      + math.sin(rad(2.0 * M)) * (0.019993 - T * 0.000101) \
      + math.sin(rad(3.0 * M)) * 0.000289

    # 7. Sun's True Longitude
    sun_true_lon = L0 + C

    # 8. Sun's Apparent Longitude
    omega = 125.04 - 1934.136 * T
    lambda_apparent = sun_true_lon - 0.00569 - 0.00478 * math.sin(rad(omega))

    # 9. Obliquity of the Ecliptic
    obliquity0 = 23.439291 - T * (46.8150 + T * (0.00059 - T * 0.001813)) / 3600.0
    obliquity = obliquity0 + 0.00256 * math.cos(rad(omega))

    # 10. Sun's Declination (Latitude of subsolar point)
    declination = deg(math.asin(math.sin(rad(obliquity)) * math.sin(rad(lambda_apparent))))

    # 11. Equation of Time (minutes)
    y = math.pow(math.tan(rad(obliquity) / 2.0), 2.0)
    eot = 4.0 * deg(y * math.sin(2.0 * rad(L0)) 
                    - 2.0 * e * math.sin(rad(M)) 
                    + 4.0 * e * y * math.sin(rad(M)) * math.cos(2.0 * rad(L0)) 
                    - 0.5 * y * y * math.sin(4.0 * rad(L0)) 
                    - 1.25 * e * e * math.sin(2.0 * rad(M)))

    # 12. Subsolar Longitude
    subsolar_lon = -(utc_hour_decimal - 12.0 + eot / 60.0) * 15.0
    subsolar_lon = (subsolar_lon + 180.0) % 360.0
    subsolar_lon -= 180.0

    return declination, subsolar_lon, eot

# Test key seasonal dates at 12:00:00 UTC
seasonal_dates = [
    ("Vernal Equinox (Spring)", 2026, 3, 20, 12, 0, 0),
    ("Summer Solstice (Summer)", 2026, 6, 21, 12, 0, 0),
    ("Autumnal Equinox (Autumn)", 2026, 9, 22, 12, 0, 0),
    ("Winter Solstice (Winter)", 2026, 12, 21, 12, 0, 0)
]

print(f"{'Seasonal Event':<26} | {'Date (UTC)':<12} | {'Declination (Tilt)':<20} | {'Subsolar Lon':<15} | {'EoT (minutes)':<15}")
print("-" * 97)

for name, y, m, d, h, mn, s in seasonal_dates:
    dec, lon, eot = calculate_solar_position(y, m, d, h, mn, s)
    date_str = f"{y}-{m:02d}-{d:02d}"
    print(f"{name:<26} | {date_str:<12} | {dec:>+18.4f}° | {lon:>13.2f}° | {eot:>13.2f} min")
