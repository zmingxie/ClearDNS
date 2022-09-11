#include <stdio.h>
#include <stdlib.h>
#include "overture.h"
#include "process.h"
#include "common.h"
#include "logger.h"
#include "cJSON.h"
#include "str.h"

void overture_dump(overture *info);
char* overture_gen_config(overture *info);

overture* overture_init(int port) { // init overture options
    overture *info = (overture*)malloc(sizeof(overture));
    info->port = port;
    info->timeout = 6; // default timeout -> 6s
    info->foreign_port = FOREIGN_PORT;
    info->domestic_port = DOMESTIC_PORT;
    info->foreign_ip_file = "/dev/null";
    info->domestic_ip_file = "/dev/null";
    info->foreign_domain_file = "/dev/null";
    info->domestic_domain_file = "/dev/null";
    return info;
}

void overture_dump(overture *info) { // show overture info in debug log
    log_debug("Overture port -> %d", info->port);
    log_debug("Overture timeout -> %d", info->timeout);
    log_debug("Overture foreign port -> %d", info->foreign_port);
    log_debug("Overture domestic port -> %d", info->domestic_port);
    log_debug("Overture foreign ip file -> %s", info->foreign_ip_file);
    log_debug("Overture domestic ip file -> %s", info->domestic_ip_file);
    log_debug("Overture foreign domain file -> %s", info->foreign_domain_file);
    log_debug("Overture domestic domain file -> %s", info->domestic_domain_file);
}

process* overture_load(overture *info, const char *file) {
    overture_dump(info);
    char *config = overture_gen_config(info); // string config (JSON format)
    char *config_file = string_join(WORK_DIR, file);
    save_file(config_file, config);
    free(config_file);
    free(config);

    process *p = (process*)malloc(sizeof(process));
    p->cmd = string_list_append(string_list_init(), OVERTURE_BIN);
    p->cmd = string_list_append(p->cmd, "-c");
    p->cmd = string_list_append(p->cmd, file);
    p->env = string_list_init();
    p->cwd = WORK_DIR;
    return p;
}

char* overture_gen_config(overture *info) { // generate json configure from overture options
    cJSON *config = cJSON_CreateObject();
    char port_str[12]; // 32-bits (MAX_LEN -> -2147483648 -> 12-bytes)
    sprintf(port_str, "%d", info->port);
    char *bind_addr = string_join(":", port_str);
    sprintf(port_str, "%d", info->foreign_port);
    char *foreign_addr = string_join("127.0.0.1:", port_str);
    sprintf(port_str, "%d", info->domestic_port);
    char *domestic_port = string_join("127.0.0.1:", port_str);

    cJSON_AddStringToObject(config, "bindAddress", bind_addr);
    cJSON_AddFalseToObject(config, "onlyPrimaryDNS");
    cJSON_AddFalseToObject(config, "ipv6UseAlternativeDNS");
    cJSON_AddTrueToObject(config, "alternativeDNSConcurrent");
    cJSON_AddStringToObject(config, "whenPrimaryDNSAnswerNoneUse", "alternativeDNS");

    cJSON *primary = cJSON_CreateObject();
    cJSON *primary_dns = cJSON_CreateArray();
    cJSON_AddStringToObject(primary, "name", "Domestic");
    cJSON_AddStringToObject(primary, "address", domestic_port);
    cJSON_AddStringToObject(primary, "protocol", "udp");
    cJSON_AddNumberToObject(primary, "timeout", info->timeout);
    cJSON_AddItemToArray(primary_dns, primary);
    cJSON_AddItemToObject(config, "primaryDNS", primary_dns);

    cJSON *alternative = cJSON_CreateObject();
    cJSON *alternative_dns = cJSON_CreateArray();
    cJSON_AddStringToObject(alternative, "name", "Foreign");
    cJSON_AddStringToObject(alternative, "address", foreign_addr);
    cJSON_AddStringToObject(alternative, "protocol", "udp");
    cJSON_AddNumberToObject(alternative, "timeout", info->timeout);
    cJSON_AddItemToArray(alternative_dns, alternative);
    cJSON_AddItemToObject(config, "alternativeDNS", alternative_dns);

    cJSON *ip_file = cJSON_CreateObject();
    cJSON_AddStringToObject(ip_file, "primary", info->domestic_ip_file);
    cJSON_AddStringToObject(ip_file, "alternative", info->foreign_ip_file);
    cJSON_AddItemToObject(config, "ipNetworkFile", ip_file);

    cJSON *domain_file = cJSON_CreateObject();
    cJSON_AddStringToObject(domain_file, "primary", info->domestic_domain_file);
    cJSON_AddStringToObject(domain_file, "alternative", info->foreign_domain_file);
    cJSON_AddStringToObject(domain_file, "matcher", "suffix-tree");
    cJSON_AddItemToObject(config, "domainFile", domain_file);

    cJSON *host_file = cJSON_CreateObject();
    cJSON_AddStringToObject(host_file, "finder", "full-map");
    cJSON_AddItemToObject(config, "hostsFile", host_file);

    char *config_str = cJSON_Print(config);
    cJSON_Delete(config); // free json object
    free(domestic_port);
    free(foreign_addr);
    free(bind_addr);
    return config_str;
}