/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2015 Intel Corporation
 */

#include <stdint.h>
#include <inttypes.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>

#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024

#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32

#define UDP_LEN 8

static int data_len = 64;

static const struct rte_eth_conf port_conf_default = {
	.rxmode = {
		.max_rx_pkt_len = ETHER_MAX_LEN,
		.ignore_offload_bitfield = 1,
	},
};

/* basicfwd.c: Basic DPDK skeleton forwarding example. */

static void construct_ipv4(struct ipv4_hdr* ipv4_header)
{
	ipv4_header->version_ihl = 0x45;
	ipv4_header->type_of_service = 0x00;
	ipv4_header->total_length = sizeof(struct ipv4_hdr) + UDP_LEN + data_len; /**< length of packet */
  ipv4_header->packet_id = 0; /**< packet ID */
  ipv4_header->fragment_offset = 0; /**< fragmentation offset */
  ipv4_header->time_to_live = 0x40; /**< time to live */
  ipv4_header->next_proto_id = 0x11; /**< protocol ID is UDP */
  ipv4_header->hdr_checksum = rte_ipv4_cksum(ipv4_header); /**< header checksum */
  ipv4_header->src_addr = 0x0a50a8c0; /**< source address */
  ipv4_header->dst_addr = 0x0650a8c0; /**< destination address */
}

static void construct_UDP(struct udp_hdr* udp_header)
{
	uint16_t port = 9001;
	udp_header->src_port = rte_cpu_to_be_16(port); /**< UDP source port. */
  udp_header->dst_port = rte_cpu_to_be_16(port); /**< UDP destination port. */
  udp_header->dgram_len = data_len; /**< UDP datagram length */
	udp_header->dgram_cksum = 0; /*do not checksum */
}

static void construct_ether(struct ether_hdr* ether_header)
{
	uint16_t type = 0x0800;
	struct ether_addr mac_addr;
	mac_addr.addr_bytes[0] = 0x00;
  mac_addr.addr_bytes[1] = 0x50;
  mac_addr.addr_bytes[2] = 0x56;
  mac_addr.addr_bytes[3] = 0xC0;
  mac_addr.addr_bytes[4] = 0x00;
  mac_addr.addr_bytes[5] = 0x02;

	ether_header->d_addr = mac_addr; /*uint8_t 	addr_bytes [ETHER_ADDR_LEN]*/
  ether_header->s_addr = mac_addr; /**< Source address. */
  ether_header->ether_type = rte_cpu_to_be_16(type); /*mean it is a IP packet */
}

static void construct_packet(char* pkt)
{
	construct_ether((struct ether_hdr*)pkt);
	construct_ipv4((struct ipv4_hdr*)(pkt + sizeof(struct ether_hdr)));
	construct_UDP((struct udp_hdr*)(pkt + sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr)));
}

/*
 * Initializes a given port using global settings and with the RX buffers
 * coming from the mbuf_pool passed as a parameter.
 */
static inline int
port_init(uint16_t port, struct rte_mempool *mbuf_pool)
{
	struct rte_eth_conf port_conf = port_conf_default;
	const uint16_t rx_rings = 1, tx_rings = 1;
	uint16_t nb_rxd = RX_RING_SIZE;
	uint16_t nb_txd = TX_RING_SIZE;
	int retval;
	uint16_t q;
	struct rte_eth_dev_info dev_info;
	struct rte_eth_txconf txconf;

	if (!rte_eth_dev_is_valid_port(port))
		return -1;

	rte_eth_dev_info_get(port, &dev_info);
	if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_MBUF_FAST_FREE)
		port_conf.txmode.offloads |=
			DEV_TX_OFFLOAD_MBUF_FAST_FREE;

	/* Configure the Ethernet device. */
	retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
	if (retval != 0)
		return retval;

	retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
	if (retval != 0)
		return retval;

	/* Allocate and set up 1 RX queue per Ethernet port. */
	for (q = 0; q < rx_rings; q++) {
		retval = rte_eth_rx_queue_setup(port, q, nb_rxd,
				rte_eth_dev_socket_id(port), NULL, mbuf_pool);
		if (retval < 0)
			return retval;
	}

	txconf = dev_info.default_txconf;
	txconf.txq_flags = ETH_TXQ_FLAGS_IGNORE;
	txconf.offloads = port_conf.txmode.offloads;
	/* Allocate and set up 1 TX queue per Ethernet port. */
	for (q = 0; q < tx_rings; q++) {
		retval = rte_eth_tx_queue_setup(port, q, nb_txd,
				rte_eth_dev_socket_id(port), &txconf);
		if (retval < 0)
			return retval;
	}

	/* Start the Ethernet port. */
	retval = rte_eth_dev_start(port);
	if (retval < 0)
		return retval;

	/* Display the port MAC address. */
	struct ether_addr addr;
	rte_eth_macaddr_get(port, &addr);
	printf("Port %u MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8
			   " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
			port,
			addr.addr_bytes[0], addr.addr_bytes[1],
			addr.addr_bytes[2], addr.addr_bytes[3],
			addr.addr_bytes[4], addr.addr_bytes[5]);

	/* Enable RX in promiscuous mode for the Ethernet device. */
	rte_eth_promiscuous_enable(port);

	return 0;
}

/*
 * The lcore main. This is the main thread that does the work, reading from
 * an input port and writing to an output port.
 */
 /*
	* Check that the port is on the same NUMA node as the polling thread
	* for best performance.
	*/
	/*struct rte_mbuf buf;
	struct ipv4_hdr ip4_hdr;
	struct udp_hdr udp_hdr;
	udp_hdr.src_port = 0;
	udp_hdr.dst_port = 22;

	ip4_hdr.src_addr = 0xc0a8500a;
	ip4_hdr.dst_addr = 0xc0a85006;

	char *data_addr =  rte_pktmbuf_mtod(&buf, char*);

	memset(data_addr, &ip4_hdr, sizeof(struct ipv4_hdr));
	memset(data_addr + sizeof(struct ipv4_hdr), &udp_hdr, sizeof(struct udp_hdr));*/
	/*
	 * Receive packets on a port and forward them on the paired
	 * port. The mapping is 0 -> 1, 1 -> 0, 2 -> 3, 3 -> 2, etc.
	 */
	 /* Run until the application is quit or killed. */
/*static __attribute__((noreturn)) void
lcore_main(void)
{
	uint16_t port;


	RTE_ETH_FOREACH_DEV(port)
		if (rte_eth_dev_socket_id(port) > 0 &&
				rte_eth_dev_socket_id(port) !=
						(int)rte_socket_id())
			printf("WARNING, port %u is on remote NUMA node to "
					"polling thread.\n\tPerformance will "
					"not be optimal.\n", port);

	printf("\nCore %u forwarding packets. [Ctrl+C to quit]\n",
			rte_lcore_id());


	for (;;) {
		RTE_ETH_FOREACH_DEV(port) {

			// Get burst of RX packets, from first port of pair.
			struct rte_mbuf *bufs[BURST_SIZE];
			const uint16_t nb_rx = rte_eth_rx_burst(port, 0,
					bufs, BURST_SIZE);

			if (unlikely(nb_rx == 0))
				continue;

			// Send burst of TX packets, to second port of pair.
			const uint16_t nb_tx = rte_eth_tx_burst(port ^ 1, 0,
					bufs, nb_rx);

			// Free any unsent packets.
			if (unlikely(nb_tx < nb_rx)) {
				uint16_t buf;
				for (buf = nb_tx; buf < nb_rx; buf++)
					rte_pktmbuf_free(bufs[buf]);
			}
		}
	}
}*/

/*
 * The main function, which does initialization and calls the per-lcore
 * functions.
 */
int
main(int argc, char *argv[])
{
	struct rte_mempool *mbuf_pool;
	//unsigned nb_ports;
	uint16_t portid;

	/* Initialize the Environment Abstraction Layer (EAL). */
	int ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

	argc -= ret;
	argv += ret;

	/* Check that there is an even number of ports to send/receive on. */
	//nb_ports = rte_eth_dev_count_avail();
	//printf("%d\n", nb_ports);
	/*if (nb_ports < 2 || (nb_ports & 1))
		rte_exit(EXIT_FAILURE, "Error: number of ports must be even\n");*/

	/* Creates a new mempool in memory to hold the mbufs. */
	mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS,
		MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

	if (mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

	portid = 0;
	if (port_init(portid, mbuf_pool) != 0)
		rte_exit(EXIT_FAILURE, "Cannot init port %"PRIu16 "\n",
				portid);
	/* Initialize all ports. */
	/*RTE_ETH_FOREACH_DEV(portid)
		if (port_init(portid, mbuf_pool) != 0)
			rte_exit(EXIT_FAILURE, "Cannot init port %"PRIu16 "\n",
					portid);*/

	if (rte_lcore_count() > 1)
		printf("\nWARNING: Too many lcores enabled. Only 1 used.\n");

	struct rte_mbuf* bufs[BURST_SIZE];
	for(int i = 0; i < BURST_SIZE; i++){
		bufs[i] =	rte_pktmbuf_alloc(mbuf_pool);
		char* pkt = rte_pktmbuf_mtod(bufs[i], char *);
		rte_pktmbuf_append(bufs[i], sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr) + 72);
		construct_packet(pkt);

	}

	rte_eth_tx_burst(0, 0, bufs, BURST_SIZE);

	/* Call lcore_main on the master core only. */
	//lcore_main();

	return 0;
}
