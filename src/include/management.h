/*
 * Copyright (C) 2021 Red Hat
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list
 * of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or other
 * materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may
 * be used to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PGEXPORTER_MANAGEMENT_H
#define PGEXPORTER_MANAGEMENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <pgexporter.h>

#include <stdbool.h>
#include <stdlib.h>

#include <openssl/ssl.h>

#define MANAGEMENT_TRANSFER_CONNECTION 1
#define MANAGEMENT_STOP                2
#define MANAGEMENT_STATUS              3
#define MANAGEMENT_DETAILS             4
#define MANAGEMENT_ISALIVE             5
#define MANAGEMENT_RESET               6
#define MANAGEMENT_RELOAD              7

/**
 * Read the management header
 * @param socket The socket descriptor
 * @param id The resulting management identifier
 * @return 0 upon success, otherwise 1
 */
int
pgexporter_management_read_header(int socket, signed char* id);

/**
 * Read the management payload
 * @param socket The socket descriptor
 * @param id The management identifier
 * @param payload_i1 The resulting integer payload
 * @param payload_i2 The resulting integer payload
 * @return 0 upon success, otherwise 1
 */
int
pgexporter_management_read_payload(int socket, signed char id, int* payload_i1, int* payload_i2);

/**
 * Management operation: Transfer a connection
 * @param slot The slot
 * @return 0 upon success, otherwise 1
 */
int
pgexporter_management_transfer_connection(int server);

/**
 * Management operation: Stop
 * @param ssl The SSL connection
 * @param socket The socket descriptor
 * @return 0 upon success, otherwise 1
 */
int
pgexporter_management_stop(SSL* ssl, int socket);

/**
 * Management operation: Status
 * @param ssl The SSL connection
 * @param socket The socket descriptor
 * @return 0 upon success, otherwise 1
 */
int
pgexporter_management_status(SSL* ssl, int socket);

/**
 * Management: Read status
 * @param socket The socket
 * @return 0 upon success, otherwise 1
 */
int
pgexporter_management_read_status(SSL* ssl, int socket);

/**
 * Management: Write status
 * @param socket The socket
 * @return 0 upon success, otherwise 1
 */
int
pgexporter_management_write_status(int socket);

/**
 * Management operation: Details
 * @param ssl The SSL connection
 * @param socket The socket
 * @return 0 upon success, otherwise 1
 */
int
pgexporter_management_details(SSL* ssl, int socket);

/**
 * Management: Read details
 * @param socket The socket
 * @return 0 upon success, otherwise 1
 */
int
pgexporter_management_read_details(SSL* ssl, int socket);

/**
 * Management: Write details
 * @param socket The socket
 * @return 0 upon success, otherwise 1
 */
int
pgexporter_management_write_details(int socket);

/**
 * Management operation: isalive
 * @param socket The socket
 * @return 0 upon success, otherwise 1
 */
int
pgexporter_management_isalive(SSL* ssl, int socket);

/**
 * Management: Read isalive
 * @param socket The socket
 * @param status The resulting status
 * @return 0 upon success, otherwise 1
 */
int
pgexporter_management_read_isalive(SSL* ssl, int socket, int* status);

/**
 * Management: Write isalive
 * @param socket The socket
 * @return 0 upon success, otherwise 1
 */
int
pgexporter_management_write_isalive(int socket);

/**
 * Management operation: Reset
 * @param ssl The SSL connection
 * @param socket The socket
 * @return 0 upon success, otherwise 1
 */
int
pgexporter_management_reset(SSL* ssl, int socket);

/**
 * Management operation: Reload
 * @param ssl The SSL connection
 * @param socket The socket
 * @return 0 upon success, otherwise 1
 */
int
pgexporter_management_reload(SSL* ssl, int socket);

#ifdef __cplusplus
}
#endif

#endif
