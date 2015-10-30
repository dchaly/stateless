#include <iostream>
#include "ns3/core-module.h"
#include "ns3/global-route-manager.h"
#include "ns3/bridge-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/csma-helper.h"

using namespace ns3;

#define EXP_NNODES 1

int main(int argc, char *argv[])
{
    std::string bandwidth0  = "1Mbps"; // Bandwidth of the link between nodes
    uint32_t segment = 1000; // Segment size
    uint32_t queue = 10; // Queue size
    uint32_t duration = 30; // Experiment duration (in seconds)
    uint32_t delay = 200; // Delay of the link between nodes (in milliseconds)
    std::string tracename = "trace.tr";
    std::string protocol = "trickles";
    
    CommandLine cmd;
    cmd.AddValue("bandwidth", "Bandwidth between nodes N0 and N1", bandwidth0);
    cmd.AddValue("delay", "Delay of the link between nodes N0 and N1", delay);
    cmd.AddValue("queue", "Queue size at nodes N0 and N1", queue);
    cmd.AddValue("segsize", "Segment size", segment);
    cmd.AddValue("duration", "Duration of the experiment", duration);
    cmd.AddValue("trace", "Trace file name", tracename);
    cmd.AddValue("protocol", "Protocol name (trickles, reno, new reno)", protocol);
    cmd.Parse(argc, argv);
    
    std::cout << "N0 (Server) --- "<< bandwidth0 << ", " << delay <<" ms --- N1 (Client)" << std::endl;
    std::cout << duration << " seconds" << std::endl;
    std::cout << "Protocol: " << protocol << std::endl;
    std::cout << "Output file: " << tracename << std::endl;
    
    /* Experiment configuration */
    Config::SetDefault("ns3::DropTailQueue::MaxPackets", UintegerValue (queue));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(segment));
    Config::SetDefault("ns3::TricklesSocket::SegmentSize", UintegerValue (segment));
    
    /* Network topology:
     Trickles Server
     10.0.0.1   10.0.0.2
         N0 ----- N1
                Trickles Sink
    */
    NodeContainer p2pNodes; // N0 and N1 nodes
    p2pNodes.Create (2);

    /* Links */
    // N0 --- N1 link
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute ("DataRate", StringValue (bandwidth0));
    p2p.SetChannelAttribute ("Delay",  TimeValue (MilliSeconds(delay)));
    NetDeviceContainer p2pDevices;
    p2pDevices = p2p.Install(p2pNodes);
    p2pDevices.Get(0)->SetMtu(1500);
    p2pDevices.Get(1)->SetMtu(1500);
    
    /* Install the IP stack. */
    if (protocol == "reno") {
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpReno"));
    }
    if (protocol == "newreno") {
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewReno"));
    }
    Config::SetDefault ("ns3::TricklesL4Protocol::SocketType", StringValue ("ns3::TricklesShieh")); // TricklesShieh by default
    Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (5000));
    Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue ("10Mbps"));
    InternetStackHelper stack;
    
    stack.Install(p2pNodes);
    
    /* IP assign. */
    Ipv4AddressHelper ipv4;
    ipv4.SetBase ("10.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer iface_p2p = ipv4.Assign (p2pDevices);
    
    /* Generate Route. */
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
    
    uint16_t app_port = 49000;
    
    ApplicationContainer sinkApps, servApps;
    if (protocol == "trickles") {
        TricklesSinkHelper sinkhelp = TricklesSinkHelper(InetSocketAddress(Ipv4Address("10.0.0.1"), app_port));
        sinkhelp.SetAttribute("PacketSize", StringValue("5000"));
        sinkhelp.SetAttribute("Remote", AddressValue(InetSocketAddress(Ipv4Address("10.0.0.1"), app_port)));
        sinkhelp.SetAttribute("DataRate", StringValue("10Mbps"));
        sinkhelp.SetAttribute("StartTime", TimeValue(Seconds(0)));
        sinkhelp.SetAttribute("StopTime", TimeValue(Seconds(duration)));
        sinkApps = sinkhelp.Install(p2pNodes.Get(1));
        TricklesServerHelper servhelp = TricklesServerHelper(InetSocketAddress(Ipv4Address::GetAny(), app_port));
        servApps = servhelp.Install(p2pNodes.Get(0));
    } else {
        PacketSinkHelper sink ("ns3::TcpSocketFactory",
                               InetSocketAddress (Ipv4Address::GetAny (), app_port));
        sinkApps = sink.Install(p2pNodes.Get(1));
        OnOffHelper server ("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address("10.0.0.2"), app_port));
        server.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
        server.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
        servApps = server.Install(p2pNodes.Get(0));
    }
    servApps.Start(Seconds(0.0));
    sinkApps.Start(Seconds(0.0));
    
    /* Simulation. */
    AsciiTraceHelper ascii;
    Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream (tracename);
    p2p.EnableAsciiAll (stream);
    
    /* Stop the simulation after x seconds. */
    uint32_t stopTime = duration;
    Simulator::Stop (Seconds (stopTime));
    /* Start and clean simulation. */
    Simulator::Run ();
    
    Simulator::Destroy ();
    
    if (protocol == "trickles") {
        Ptr<TricklesSink> sink1 = DynamicCast<TricklesSink> (sinkApps.Get (0));
        std::cout << "Total Bytes Received: " << sink1->GetTotalRx () << " (" << sink1->GetTotalRq() << " requested)"<< std::endl;
    } else {
        Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkApps.Get (0));
        std::cout << "Total Bytes Received: " << sink1->GetTotalRx () << std::endl;
    }

}
