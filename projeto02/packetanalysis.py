from scapy.all import *
import nest_asyncio
import matplotlib.pyplot as plt
nest_asyncio.apply()

# This code was made in order to analyse the packets capture during the exchange between 2 hosts
# h1 and h3 and other 2 h2 and h4. It reads the .pcap files with scapy's rdpcap() and creates
# graphs of the results found with matplotlib.
def count_icmp_packets(pcap_file):
    # Counts the number of ICMP packets in the traffic.
    packets = rdpcap(pcap_file)
    icmp_packets = [pkt for pkt in packets if ICMP in pkt]
    return len(icmp_packets)

def count_total_packets(pcap_file):
    # Count the total number of packets.
    packets = rdpcap(pcap_file)
    return len(packets)

def find_unique_ips(pcap_file):
    # Identify unique source and destination IP addresses.
    packets = rdpcap(pcap_file)
    src_ips = set()
    dst_ips = set()

    for pkt in packets:
        if IP in pkt:
            src_ips.add(pkt[IP].src)
            dst_ips.add(pkt[IP].dst)

    return src_ips, dst_ips

def calculate_average_throughput(pcap_file):
    # Calculate average throughput as total bytes divided by total capture duration.
    packets = rdpcap(pcap_file)
    total_bytes = sum(len(pkt) for pkt in packets)
    total_time = packets[-1].time - packets[0].time

    throughput = total_bytes / total_time if total_time > 0 else 0
    return throughput

def calculate_avg_packet_interval(pcap_file):
    # Calculate the average time interval between consecutive packets.
    packets = rdpcap(pcap_file)
    intervals = [packets[i].time - packets[i-1].time for i in range(1, len(packets))]
    return sum(intervals) / len(intervals) if intervals else 0

def identify_packet_types(pcap_file):
    # Identify packet types (Ether, IP, ARP) based on Scapy's classification.
    packets = rdpcap(pcap_file)
    packet_types = {}

    for pkt in packets:
        pkt_type = pkt.__class__.__name__
        packet_types[pkt_type] = packet_types.get(pkt_type, 0) + 1

    return packet_types

def calculate_icmp_packet_loss(pcap_file):
    # Estimate ICMP packet loss by counting echo requests and replies.
    packets = rdpcap(pcap_file)
    echo_requests = 0
    echo_replies = 0

    for pkt in packets:
        if ICMP in pkt:
            if pkt[ICMP].type == 8:  # Echo Request
                echo_requests += 1
            elif pkt[ICMP].type == 0:  # Echo Reply
                echo_replies += 1

    lost = echo_requests - echo_replies
    # Take percentage
    loss_percent = (lost / echo_requests * 100) if echo_requests > 0 else 0

    return echo_requests, echo_replies, lost, loss_percent

# Plotting Functions
def plot_interval_metrics(pcap_files):
    # Plot packet interval and throughput over time
    plt.figure(figsize=(14, 10))

    # Packet Interval vs Time
    plt.subplot(2, 1, 1)
    for pcap_file in pcap_files:
        packets = rdpcap(pcap_file)
        times = [pkt.time - packets[0].time for pkt in packets]
        packet_intervals = [times[i] - times[i-1] for i in range(1, len(times))]
        avg_packet_interval = calculate_avg_packet_interval(pcap_file)

        plt.plot(times[1:], packet_intervals, label=f'{pcap_file}')
        plt.axhline(y=avg_packet_interval, linestyle='--', label=f'Avg Interval {pcap_file}')

    plt.xlabel('Time (s)')
    plt.ylabel('Packet Interval (s)')
    plt.title('Packet Interval vs Time')
    plt.legend()

    # Throughput vs Time
    plt.subplot(2, 1, 2)
    for pcap_file in pcap_files:
        packets = rdpcap(pcap_file)
        times = [pkt.time - packets[0].time for pkt in packets]
        throughput_intervals = [(len(packets[i]))/(times[i] - times[i-1]) if (times[i] - times[i-1]) > 0 else 0 for i in range(1, len(times))]
        plt.plot(times[1:], throughput_intervals, label=f'{pcap_file}')

    plt.xlabel('Time (s)')
    plt.ylabel('Throughput (bytes/s)')
    plt.title('Throughput vs Time')
    plt.legend()

    plt.tight_layout()
    plt.show()


def plot_cumulative_throughput(pcap_files):
    # Plot cumulative throughput (total bytes transferred) over time
    plt.figure(figsize=(10, 6))

    for pcap_file in pcap_files:
        packets = rdpcap(pcap_file)
        base_time = packets[0].time
        times = []
        cumulative_bytes = []
        total_bytes = 0

        for pkt in packets:
            if IP in pkt:
                pkt_time = pkt.time - base_time
                pkt_len = len(pkt)
                total_bytes += pkt_len
                times.append(pkt_time)
                cumulative_bytes.append(total_bytes)

        label = "h1 to h3" if "h1h3" in pcap_file else "h2 to h4"
        plt.plot(times, cumulative_bytes, label=label)

    plt.xlabel("Time (s)")
    plt.ylabel("Cumulative Throughput (bytes)")
    plt.title("Cumulative Throughput over Time")
    plt.legend()
    plt.grid(True)
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    # List of .pcap files captured to be analyzed
    pcap_files = ["icmph1h3.pcap", "icmph2h4.pcap"]

    for pcap_file in pcap_files:
        print(f"\nAnalysis of {pcap_file}:")

        # Perform individual analyses for each capture file
        total_packets = count_total_packets(pcap_file)
        icmp_packets = count_icmp_packets(pcap_file)
        src_ips, dst_ips = find_unique_ips(pcap_file)
        avg_throughput = calculate_average_throughput(pcap_file)
        avg_interval = calculate_avg_packet_interval(pcap_file)
        packet_types = identify_packet_types(pcap_file)
        echo_req, echo_rep, lost, loss_pct = calculate_icmp_packet_loss(pcap_file)

        # Print results for each metric
        print("Total packets:", total_packets)
        print("Total ICMP packets:", icmp_packets)
        print("Unique source IPs:", src_ips)
        print("Unique destination IPs:", dst_ips)
        print("Average throughput (bytes/s): {:.2f}".format(avg_throughput))
        print("Average packet interval (s): {:.2f}".format(avg_interval))
        print("Packet types captured:", packet_types)
        print("ICMP Echo Requests:", echo_req)
        print("ICMP Echo Replies:", echo_rep)
        print("ICMP Packet Loss (%): {:.2f}%".format(loss_pct))

    # Generate plots for all capture files
    plot_interval_metrics(pcap_files)
    plot_cumulative_throughput(pcap_files)
