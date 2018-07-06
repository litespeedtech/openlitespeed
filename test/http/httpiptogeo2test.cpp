/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2018  LiteSpeed Technologies, Inc.                 *
*                                                                            *
*    This program is free software: you can redistribute it and/or modify    *
*    it under the terms of the GNU General Public License as published by    *
*    the Free Software Foundation, either version 3 of the License, or       *
*    (at your option) any later version.                                     *
*                                                                            *
*    This program is distributed in the hope that it will be useful,         *
*    but WITHOUT ANY WARRANTY; without even the implied warranty of          *
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the            *
*    GNU General Public License for more details.                            *
*                                                                            *
*    You should have received a copy of the GNU General Public License       *
*    along with this program. If not, see http://www.gnu.org/licenses/.      *
*****************************************************************************/
#ifdef RUN_TEST
#include <util/env.h>
#include <http/iptogeo2.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include "unittest-cpp/UnitTest++.h"
#include <string.h>

#ifdef ENABLE_IPTOGEO2
SUITE(IpToGeo2Test)
{
    
TEST(IpToGeo2Test_Empty)
{
    IpToGeo2 ipToGeo2;
    GeoInfo *geoInfo2 = NULL;
    
    fprintf(stderr,"IpToGeo2Test_Empty begins\n");
    CHECK((geoInfo2 = ipToGeo2.lookUp((const char *)"88.252.206.167")) == NULL);
    if (geoInfo2)
        delete geoInfo2;
}


int getdb(const char *db, char *finaldb, int *created)
{
    char fulldb[300];
    char compressed[300];
    char command[350];
    char line[350];
    char tar[350];
    char file_so_far[350] = { 0 };
    
    if (created)
        *created = 0;
    snprintf(fulldb, sizeof(fulldb), "%s.mmdb", db);
    if (finaldb)
        strcpy(finaldb, fulldb);
    if (access(fulldb,0) == 0)
        return 1;
    snprintf(compressed, sizeof(compressed), "%s.tar.gz", db);
    snprintf(command, sizeof(command), "wget https://geolite.maxmind.com/download/geoip/database/%s", compressed);
    printf("File not found; downloading: %s\n", fulldb);
    FILE *out;
    if (!(out = popen(command, "r")))
    {
        printf("Error in popen of %s: %s\n", command, strerror(errno));
        return 0;
    }
    while (fgets(line, sizeof(line), out))
        printf("   -> %s", line);
    int status;
    status = pclose(out);
    if (status != 0)
    {
        printf("Error getting file: %s, %d\n", command, status);
        return 0;
    }
    strcpy(file_so_far, compressed);
    snprintf(command, sizeof(command), "gunzip ./%s", compressed);
    printf("   %s\n", command);
    if (!(out = popen(command, "r")))
    {
        printf("Error in popen of %s: %s\n", command, strerror(errno));
        unlink(file_so_far);
        return 0;
    }
    while (fgets(line, sizeof(line), out))
        printf("   -> %s", line);
    status = pclose(out);
    if (status != 0)
    {
        printf("Error decompressing file: %s, %d\n", command, status);
        unlink(file_so_far);
        return 0;
    }
    snprintf(file_so_far, sizeof(file_so_far), "%s.tar", db);
    strcpy(tar, file_so_far);
    snprintf(command, sizeof(command), "tar xvf %s --wildcards '*/%s' --strip-components=1",
             file_so_far, fulldb);
    printf("   %s\n", command);
    if (!(out = popen(command, "r")))
    {
        printf("Error in popen of %s: %s\n", command, strerror(errno));
        unlink(file_so_far);
        return 0;
    }
    while (fgets(line, sizeof(line), out))
        printf("   -> %s", line);
    status = pclose(out);
    unlink(file_so_far);
    if (status != 0)
    {
        printf("Error extracting file: %s, %d\n", command, status);
        return 0;
    }
    strcpy(file_so_far, fulldb);
    if (finaldb)
        strcpy(finaldb, fulldb);
    if (created)
        *created = 1;
    return 1;
}


TEST(IpToGeo2Test_LoadOnly)
{
    IpToGeo2 ipToGeo2;
    GeoInfo *geoInfo2 = NULL;
    
    fprintf(stderr,"IpToGeo2Test_LoadOnly begins\n");
    CHECK((geoInfo2 = ipToGeo2.lookUp((const char *)"88.252.206.167")) == NULL);
    if (geoInfo2)
        delete geoInfo2;
}

    
TEST(IpToGeo2Test_GetOnly)
{
    IpToGeo2 ipToGeo2;
    char     finaldb[350] = { 0 };
    int      created = 0;
    
    fprintf(stderr,"IpToGeo2Test_GetOnly begins\n");
    CHECK(getdb("GeoLite2-Country", finaldb, &created) != 0);
    CHECK(ipToGeo2.setGeoIpDbFile(finaldb, "COUNTRY_DB") == 0);
    //if (created)
    //    unlink(finaldb);
}

    
TEST(IpToGeo2Test_GetAndLookup)
{
    IpToGeo2 ipToGeo2;
    GeoInfo *geoInfo2 = NULL;
    char     finaldb[350] = { 0 };
    int      created = 0;
    const char *res;
    
    fprintf(stderr,"IpToGeo2Test_GetAndLookup begins\n");
    CHECK(getdb("GeoLite2-Country", finaldb, &created) != 0);
    CHECK(ipToGeo2.setGeoIpDbFile(finaldb, "COUNTRY_DB") == 0);
    geoInfo2 = ipToGeo2.lookUp((const char *)"88.252.206.167");
    CHECK(geoInfo2 != NULL);
    res = geoInfo2->getGeoEnv("GEOIP_COUNTRY_CODE");
    CHECK(res != NULL);
    res = geoInfo2->getGeoEnv("GEOIP_COUNTRY_NAME");
    CHECK(res != NULL);
    // First
    CHECK((res = geoInfo2->getGeoEnv("GEOIP_CONTINENT_CODE")) != NULL);
    // Not found
    if (geoInfo2)
        delete geoInfo2;
    if (created)
        unlink(finaldb);
}

    
TEST(IpToGeo2Test_3DBs)
{
    IpToGeo2 ipToGeo2;
    GeoInfo *geoInfo2 = NULL;
    char     finaldbCountry[350] = { 0 };
    char     finaldbCity[350] = { 0 };
    char     finaldbASN[350] = { 0 };
    int      createdCountry = 0;
    int      createdCity = 0;
    int      createdASN = 0;
    const char *res;
    
    fprintf(stderr,"IpToGeo2Test_3DBs begins\n");
    CHECK(getdb("GeoLite2-Country", finaldbCountry, &createdCountry) != 0);
    CHECK(ipToGeo2.setGeoIpDbFile(finaldbCountry, "COUNTRY_DB") == 0);
    CHECK(getdb("GeoLite2-City", finaldbCity, &createdCity) != 0);
    CHECK(ipToGeo2.setGeoIpDbFile(finaldbCity, "CITY_DB") == 0);
    CHECK(getdb("GeoLite2-ASN", finaldbASN, &createdASN) != 0);
    CHECK(ipToGeo2.setGeoIpDbFile(finaldbASN, "ASN_DB") == 0);
    geoInfo2 = ipToGeo2.lookUp((const char *)"88.252.206.167");
    CHECK(geoInfo2 != NULL);
    res = geoInfo2->getGeoEnv("GEOIP_COUNTRY_CODE");
    CHECK(res != NULL);
    res = geoInfo2->getGeoEnv("GEOIP_COUNTRY_NAME");
    CHECK(res != NULL);
    // First
    CHECK((res = geoInfo2->getGeoEnv("GEOIP_CONTINENT_CODE")) != NULL);
    // Not found
    CHECK(geoInfo2->getGeoEnv("GEOIP_BADNAME") == NULL);
    CHECK(geoInfo2->getGeoEnv("GEOIP_POSTAL_CODE") != NULL);
    CHECK(geoInfo2->getGeoEnv("GEOIP_METRO_CODE") == NULL); // No there for this IP
    CHECK(geoInfo2->getGeoEnv("GEOIP_LATITUDE") != NULL);
    CHECK(geoInfo2->getGeoEnv("GEOIP_LONGITUDE") != NULL);
    CHECK(geoInfo2->getGeoEnv("GEOIP_CITY") != NULL);
    CHECK(geoInfo2->getGeoEnv("GEOIP_ISP") != NULL);
    if (geoInfo2)
        delete geoInfo2;

    /*
    if (createdCountry)
        unlink(finaldbCountry);
    if (createdCity)
        unlink(finaldbCity);
    if (createdASN)
        unlink(finaldbASN);
    */
}

    
TEST(IpToGeo2Test_2LookUps)
{
    IpToGeo2 ipToGeo2;
    GeoInfo *geoInfo2 = NULL;
    char     finaldbCountry[350] = { 0 };
    char     finaldbCity[350] = { 0 };
    char     finaldbASN[350] = { 0 };
    int      createdCountry = 0;
    int      createdCity = 0;
    int      createdASN = 0;
    const char *res;
    
    fprintf(stderr,"IpToGeo2Test_2LookUps begins\n");
    CHECK(getdb("GeoLite2-Country", finaldbCountry, &createdCountry) != 0);
    CHECK(ipToGeo2.setGeoIpDbFile(finaldbCountry, "COUNTRY_DB") == 0);
    CHECK(getdb("GeoLite2-City", finaldbCity, &createdCity) != 0);
    CHECK(ipToGeo2.setGeoIpDbFile(finaldbCity, "CITY_DB") == 0);
    CHECK(getdb("GeoLite2-ASN", finaldbASN, &createdASN) != 0);
    CHECK(ipToGeo2.setGeoIpDbFile(finaldbASN, "ASN_DB") == 0);
    geoInfo2 = ipToGeo2.lookUp((const char *)"88.252.206.167");
    CHECK(geoInfo2 != NULL);
    res = geoInfo2->getGeoEnv("GEOIP_COUNTRY_CODE");
    CHECK(res != NULL);
    res = geoInfo2->getGeoEnv("GEOIP_COUNTRY_NAME");
    CHECK(res != NULL);
    // First
    CHECK((res = geoInfo2->getGeoEnv("GEOIP_CONTINENT_CODE")) != NULL);
    // Not found
    CHECK(geoInfo2->getGeoEnv("GEOIP_BADNAME") == NULL);
    CHECK(geoInfo2->getGeoEnv("GEOIP_POSTAL_CODE") != NULL);
    CHECK(geoInfo2->getGeoEnv("GEOIP_METRO_CODE") == NULL); // No there for this IP
    CHECK(geoInfo2->getGeoEnv("GEOIP_LATITUDE") != NULL);
    CHECK(geoInfo2->getGeoEnv("GEOIP_LONGITUDE") != NULL);
    CHECK(geoInfo2->getGeoEnv("GEOIP_CITY") != NULL);
    CHECK(geoInfo2->getGeoEnv("GEOIP_ISP") != NULL);
    if (geoInfo2)
    {
        delete geoInfo2;
        geoInfo2 = NULL;
    }
    
    unsigned char IP[4] = { 52,55,120,73 }; // Do it as an int (www.litespeedtech.com)
    geoInfo2 = ipToGeo2.lookUp(*(uint32_t *)&IP[0]);
    CHECK(geoInfo2 != NULL);
    res = geoInfo2->getGeoEnv("GEOIP_COUNTRY_CODE");
    CHECK(res && !strcasecmp(res,"US"));
    res = geoInfo2->getGeoEnv("GEOIP_COUNTRY_NAME");
    CHECK(res && !strcasecmp(res,"United States"));
    // First
    CHECK(((res = geoInfo2->getGeoEnv("GEOIP_CONTINENT_CODE")) != NULL) && 
          !strcasecmp(res,"NA"));;
    CHECK(geoInfo2->getGeoEnv("GEOIP_BADNAME") == NULL);
    CHECK((res = geoInfo2->getGeoEnv("GEOIP_POSTAL_CODE")) != NULL);
    CHECK(geoInfo2->getGeoEnv("GEOIP_METRO_CODE") != NULL); // No there for this IP
    CHECK(geoInfo2->getGeoEnv("GEOIP_LATITUDE") != NULL);
    CHECK(geoInfo2->getGeoEnv("GEOIP_LONGITUDE") != NULL);
    CHECK(geoInfo2->getGeoEnv("GEOIP_CITY") != NULL);
    CHECK(geoInfo2->getGeoEnv("GEOIP_ISP") != NULL);
    if (geoInfo2)
        delete geoInfo2;
    
    /*
    if (createdCountry)
        unlink(finaldbCountry);
    if (createdCity)
        unlink(finaldbCity);
    if (createdASN)
        unlink(finaldbASN);
    */
}

    
TEST(IpToGeo2Test_SetEnvNoFile)
{
    IpToGeo2 ipToGeo2;
    
    fprintf(stderr,"IpToGeo2Test_SetEnvNoFile\n");
    CHECK(ipToGeo2.configSetEnv("MM_COUNTRY_CODE CITY_DB/country/iso_code") == -1);
} // IpToGeo2Test_SetEnvNoFile


TEST(IpToGeo2Test_SetEnvBadDB)
{
    IpToGeo2 ipToGeo2;
    char     finaldb[350] = { 0 };
    int      created = 0;
    
    fprintf(stderr,"IpToGeo2Test_SetEnvBadDB begins\n");
    CHECK(getdb("GeoLite2-Country", finaldb, &created) != 0);
    CHECK(ipToGeo2.setGeoIpDbFile(finaldb, "COUNTRY_DB") == 0);
    CHECK(ipToGeo2.configSetEnv("COUNTRY_CODE COUNTRY_DB/country/iso_code") == 0);
    CHECK(ipToGeo2.configSetEnv("REGION CITY_DB/subdivisions/iso_code") == -1);
    //if (created)
    //    unlink(finaldb);
}

TEST(IpToGeo2Test_SetEnvLookupOne)
{
    IpToGeo2 ipToGeo2;
    GeoInfo *geoInfo2 = NULL;
    char     finaldb[350] = { 0 };
    int      created = 0;
    const char *res;
    
    fprintf(stderr,"IpToGeo2Test_SetEnvBadDB begins\n");
    CHECK(getdb("GeoLite2-Country", finaldb, &created) != 0);
    CHECK(ipToGeo2.setGeoIpDbFile(finaldb, "COUNTRY_DB") == 0);
    CHECK(ipToGeo2.configSetEnv("COUNTRY_CODE COUNTRY_DB/country/iso_code") == 0);
    geoInfo2 = ipToGeo2.lookUp((const char *)"88.252.206.167");
    CHECK(geoInfo2 != NULL);
    res = geoInfo2->getGeoEnv("COUNTRY_CODE");
    CHECK(res && !strcasecmp(res,"TR"));
    CHECK(geoInfo2->getGeoEnv("REGION") == NULL);
    if (geoInfo2)
        delete geoInfo2;
    //if (created)
    //    unlink(finaldb);
}

TEST(IpToGeo2Test_SetEnv3DBs)
{
    IpToGeo2 ipToGeo2;
    GeoInfo *geoInfo2;
    char     finaldbCountry[350] = { 0 };
    char     finaldbCity[350] = { 0 };
    char     finaldbASN[350] = { 0 };
    int      createdCountry = 0;
    int      createdCity = 0;
    int      createdASN = 0;
    const char *res;
    
    fprintf(stderr,"IpToGeo2Test_SetEnv3DBs begins\n");
    CHECK(getdb("GeoLite2-Country", finaldbCountry, &createdCountry) != 0);
    CHECK(ipToGeo2.setGeoIpDbFile(finaldbCountry, "COUNTRY_DB") == 0);
    CHECK(ipToGeo2.configSetEnv("MM_COUNTRY_CODE COUNTRY_DB/country/iso_code") == 0);
    CHECK(ipToGeo2.configSetEnv("MM_COUNTRY_NAME COUNTRY_DB/country/names/en") == 0);
    CHECK(ipToGeo2.configSetEnv("MM_CONTINENT_CODE COUNTRY_DB/continent/code") == 0);
    
    CHECK(getdb("GeoLite2-City", finaldbCity, &createdCity) != 0);
    CHECK(ipToGeo2.setGeoIpDbFile(finaldbCity, "CITY_DB") == 0);
    CHECK(ipToGeo2.configSetEnv("MM_COUNTRY_CODE CITY_DB/country/iso_code") == 0);
    CHECK(ipToGeo2.configSetEnv("MM_COUNTRY_NAME CITY_DB/country/names/en") == 0);
    CHECK(ipToGeo2.configSetEnv("MM_CITY_NAME CITY_DB/city/names/en") == 0);
    CHECK(ipToGeo2.configSetEnv("MM_LONGITUDE CITY_DB/location/longitude") == 0);
    CHECK(ipToGeo2.configSetEnv("MM_LATITUDE CITY_DB/location/latitude") == 0);
    
    CHECK(getdb("GeoLite2-ASN", finaldbASN, &createdASN) != 0);
    CHECK(ipToGeo2.setGeoIpDbFile(finaldbASN, "ASN_DB") == 0);
    CHECK(ipToGeo2.configSetEnv("MM_ASN ASN_DB/autonomous_system_number") == 0);
    CHECK(ipToGeo2.configSetEnv("MM_ASORG ASN_DB/autonomous_system_organization") == 0);
    
    geoInfo2 = ipToGeo2.lookUp((const char *)"88.252.206.167");
    CHECK(geoInfo2 != NULL);
    res = geoInfo2->getGeoEnv("MM_COUNTRY_CODE");
    CHECK(res != NULL);
    res = geoInfo2->getGeoEnv("MM_COUNTRY_NAME");
    CHECK(res != NULL);
    // First
    CHECK((res = geoInfo2->getGeoEnv("MM_CONTINENT_CODE")) != NULL);
    // Not found
    CHECK(geoInfo2->getGeoEnv("MM_BADNAME") == NULL);
    CHECK(geoInfo2->getGeoEnv("MM_LATITUDE") != NULL);
    CHECK(geoInfo2->getGeoEnv("MM_LONGITUDE") != NULL);
    CHECK(geoInfo2->getGeoEnv("MM_CITY_NAME") != NULL);
    CHECK(geoInfo2->getGeoEnv("MM_ASN") != NULL);
    if (geoInfo2)
        delete geoInfo2;

    /*
    if (createdCountry)
        unlink(finaldbCountry);
    if (createdCity)
        unlink(finaldbCity);
    if (createdASN)
        unlink(finaldbASN);
    */
}

    
TEST(IpToGeo2Test_SetEnv2LookUps)
{
    IpToGeo2 ipToGeo2;
    GeoInfo *geoInfo2 = NULL;
    char     finaldbCountry[350] = { 0 };
    char     finaldbCity[350] = { 0 };
    char     finaldbASN[350] = { 0 };
    int      createdCountry = 0;
    int      createdCity = 0;
    int      createdASN = 0;
    const char *res;
    
    fprintf(stderr,"IpToGeo2Test_SetEnv2LookUps begins\n");
    CHECK(getdb("GeoLite2-ASN", finaldbASN, &createdASN) != 0);
    CHECK(ipToGeo2.setGeoIpDbFile(finaldbASN, "ASN_DB") == 0);
    CHECK(ipToGeo2.configSetEnv("MM_ASN ASN_DB/autonomous_system_number") == 0);
    CHECK(ipToGeo2.configSetEnv("MM_ASORG ASN_DB/autonomous_system_organization") == 0);
    
    CHECK(getdb("GeoLite2-Country", finaldbCountry, &createdCountry) != 0);
    CHECK(ipToGeo2.setGeoIpDbFile(finaldbCountry, "COUNTRY_DB") == 0);
    CHECK(ipToGeo2.configSetEnv("MM_COUNTRY_CODE COUNTRY_DB/country/iso_code") == 0);
    CHECK(ipToGeo2.configSetEnv("MM_COUNTRY_NAME COUNTRY_DB/country/names/en") == 0);
    CHECK(ipToGeo2.configSetEnv("MM_CONTINENT_CODE COUNTRY_DB/continent/code") == 0);
    
    CHECK(getdb("GeoLite2-City", finaldbCity, &createdCity) != 0);
    CHECK(ipToGeo2.setGeoIpDbFile(finaldbCity, "CITY_DB") == 0);
    CHECK(ipToGeo2.configSetEnv("MM_COUNTRY_CODE CITY_DB/country/iso_code") == 0);
    CHECK(ipToGeo2.configSetEnv("MM_COUNTRY_NAME CITY_DB/country/names/en") == 0);
    CHECK(ipToGeo2.configSetEnv("MM_CITY_NAME CITY_DB/city/names/en") == 0);
    CHECK(ipToGeo2.configSetEnv("MM_LONGITUDE CITY_DB/location/longitude") == 0);
    CHECK(ipToGeo2.configSetEnv("MM_LATITUDE CITY_DB/location/latitude") == 0);
    
    geoInfo2 = ipToGeo2.lookUp((const char *)"88.252.206.167");
    CHECK(geoInfo2 != NULL);
    res = geoInfo2->getGeoEnv("MM_COUNTRY_CODE");
    CHECK(res != NULL);
    res = geoInfo2->getGeoEnv("MM_COUNTRY_NAME");
    CHECK(res != NULL);
    // First
    CHECK((res = geoInfo2->getGeoEnv("MM_CONTINENT_CODE")) != NULL);
    // Not found
    CHECK(geoInfo2->getGeoEnv("MM_BADNAME") == NULL);
    CHECK(geoInfo2->getGeoEnv("MM_LATITUDE") != NULL);
    CHECK(geoInfo2->getGeoEnv("MM_LONGITUDE") != NULL);
    CHECK(geoInfo2->getGeoEnv("MM_CITY_NAME") != NULL);
    CHECK(geoInfo2->getGeoEnv("MM_ASN") != NULL);
    if (geoInfo2)
    {
        delete geoInfo2;
        geoInfo2 = NULL;
    }
    
    
    unsigned char IP[4] = { 52,55,120,73 }; // Do it as an int (www.litespeedtech.com)
    geoInfo2 = ipToGeo2.lookUp(*(uint32_t *)&IP[0]);
    CHECK(geoInfo2 != NULL);
    res = geoInfo2->getGeoEnv("MM_COUNTRY_CODE");
    CHECK(res && !strcasecmp(res,"US"));
    res = geoInfo2->getGeoEnv("MM_COUNTRY_NAME");
    CHECK(res && !strcasecmp(res,"United States"));
    // First
    CHECK(((res = geoInfo2->getGeoEnv("MM_CONTINENT_CODE")) != NULL) && 
          !strcasecmp(res,"NA"));;
    CHECK(geoInfo2->getGeoEnv("MM_BADNAME") == NULL);
    CHECK((res = geoInfo2->getGeoEnv("MM_POSTAL_CODE")) == NULL);
    CHECK(geoInfo2->getGeoEnv("MM_METRO_CODE") == NULL); 
    CHECK(geoInfo2->getGeoEnv("MM_LATITUDE") != NULL);
    CHECK(geoInfo2->getGeoEnv("MM_LONGITUDE") != NULL);
    CHECK(geoInfo2->getGeoEnv("MM_CITY_NAME") != NULL);
    CHECK(geoInfo2->getGeoEnv("MM_ASN") != NULL);
    if (geoInfo2)
        delete geoInfo2;

    /*
    if (createdCountry)
        unlink(finaldbCountry);
    if (createdCity)
        unlink(finaldbCity);
    if (createdASN)
        unlink(finaldbASN);
    */
}


TEST(IpToGeo2Test_SetEnvLookUpV6)
{
    IpToGeo2 ipToGeo2;
    GeoInfo *geoInfo2 = NULL;
    char     finaldb[350] = { 0 };
    int      created = 0;
    const char *res;
    
    fprintf(stderr,"IpToGeo2Test_SetEnvLookUpV6 begins\n");
    CHECK(getdb("GeoLite2-Country", finaldb, &created) != 0);
    CHECK(ipToGeo2.setGeoIpDbFile(finaldb, "COUNTRY_DB") == 0);
    CHECK(ipToGeo2.configSetEnv("COUNTRY_CODE COUNTRY_DB/country/iso_code") == 0);
    geoInfo2 = ipToGeo2.lookUpV6("2607:f0d0:3:8::4");
    CHECK(geoInfo2 != NULL);
    res = geoInfo2->getGeoEnv("COUNTRY_CODE");
    CHECK(res != NULL);
    CHECK(geoInfo2->getGeoEnv("REGION") == NULL);
    if (geoInfo2)
        delete geoInfo2;
    //if (created)
    //    unlink(finaldb);
}


TEST(IpToGeo2Test_SetEnvLookupFailed)
{
    IpToGeo2 ipToGeo2;
    GeoInfo *geoInfo2 = NULL;
    char     finaldb[350] = { 0 };
    int      created = 0;
    
    fprintf(stderr,"IpToGeo2Test_SetEnvLookupFailed begins\n");
    CHECK(getdb("GeoLite2-Country", finaldb, &created) != 0);
    CHECK(ipToGeo2.setGeoIpDbFile(finaldb, "COUNTRY_DB") == 0);
    CHECK(ipToGeo2.configSetEnv("COUNTRY_CODE COUNTRY_DB/country/iso_code") == 0);
    geoInfo2 = ipToGeo2.lookUp("127.0.0.1");
    CHECK(geoInfo2 == NULL);
    if (geoInfo2)
        delete geoInfo2;
    //if (created)
    //    unlink(finaldb);
}


TEST(IpToGeo2Test_3DBsAddGeoEnv)
{
    IpToGeo2 ipToGeo2;
    GeoInfo *geoInfo2 = NULL;
    char     finaldbCountry[350] = { 0 };
    char     finaldbCity[350] = { 0 };
    char     finaldbASN[350] = { 0 };
    int      createdCountry = 0;
    int      createdCity = 0;
    int      createdASN = 0;
    int      ret;
    
    fprintf(stderr,"IpToGeo2Test_3DBsAddGeoEnv begins\n");
    CHECK(getdb("GeoLite2-Country", finaldbCountry, &createdCountry) != 0);
    CHECK(ipToGeo2.setGeoIpDbFile(finaldbCountry, "COUNTRY_DB") == 0);
    CHECK(ipToGeo2.configSetEnv("MM_COUNTRY_CODE COUNTRY_DB/country/iso_code") == 0);
    CHECK(ipToGeo2.configSetEnv("MM_COUNTRY_NAME COUNTRY_DB/country/names/en") == 0);
    CHECK(ipToGeo2.configSetEnv("MM_CONTINENT_CODE COUNTRY_DB/continent/code") == 0);
    
    CHECK(getdb("GeoLite2-City", finaldbCity, &createdCity) != 0);
    CHECK(ipToGeo2.setGeoIpDbFile(finaldbCity, "CITY_DB") == 0);
    CHECK(ipToGeo2.configSetEnv("MM_COUNTRY_CODE CITY_DB/country/iso_code") == 0);
    CHECK(ipToGeo2.configSetEnv("MM_COUNTRY_NAME CITY_DB/country/names/en") == 0);
    CHECK(ipToGeo2.configSetEnv("MM_CITY_NAME CITY_DB/city/names/en") == 0);
    CHECK(ipToGeo2.configSetEnv("MM_LONGITUDE CITY_DB/location/longitude") == 0);
    CHECK(ipToGeo2.configSetEnv("MM_LATITUDE CITY_DB/location/latitude") == 0);
    
    CHECK(getdb("GeoLite2-ASN", finaldbASN, &createdASN) != 0);
    CHECK(ipToGeo2.setGeoIpDbFile(finaldbASN, "ASN_DB") == 0);
    CHECK(ipToGeo2.configSetEnv("MM_ASN ASN_DB/autonomous_system_number") == 0);
    CHECK(ipToGeo2.configSetEnv("MM_ASORG ASN_DB/autonomous_system_organization") == 0);
    
    geoInfo2 = ipToGeo2.lookUp((const char *)"88.252.206.167");
    Env iEnv;
    ret = geoInfo2->addGeoEnv(&iEnv);
    CHECK(ret >= 0);
    if (ret >= 0)
    {
        CHECK(iEnv.find("MM_COUNTRY_CODE") != NULL);
        CHECK(iEnv.find("MM_COUNTRY_NAME") != NULL);
        CHECK(iEnv.find("MM_CONTINENT_CODE") != NULL);
        CHECK(iEnv.find("MM_CITY_NAME") != NULL);
        CHECK(iEnv.find("MM_LONGITUDE") != NULL);
        CHECK(iEnv.find("MM_LATITUDE") != NULL);
        CHECK(iEnv.find("MM_ASN") != NULL);
        CHECK(iEnv.find("MM_ASORG") != NULL);
        CHECK(iEnv.find("MM_BADNAME") == NULL);
    }
    if (geoInfo2)
        delete geoInfo2;
    /*
    if (createdCountry)
        unlink(finaldbCountry);
    if (createdCity)
        unlink(finaldbCity);
    if (createdASN)
        unlink(finaldbASN);
    */
}

    
  
} // suite
#endif

#endif

