/*
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"


#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace ns3;

int main (int argc, char *argv[])
{
  LogComponentEnable("UdpClient", LOG_LEVEL_INFO);
  LogComponentEnable("UdpServer", LOG_LEVEL_INFO);

  double distance=10.0;
  double interval=0.00015;
  int n=1, packetSize=1024;

  CommandLine cmd;

  cmd.AddValue ("interval", "Interval entre les paquets", interval);
  cmd.AddValue ("distance", "Distance entre les deux noeuds", distance);
  cmd.AddValue ("packetSize", "Taille des paquets en octets", packetSize);

  cmd.Parse (argc, argv);

  NodeContainer nodes;
  NodeContainer nodesAP;
  nodesAP.Create(1);//AP 
  nodes.Create (n);//noeud ad hoc recepteur - n stations

  /*** MAC a 1Mbit/s ***/
  // The below set of helpers will help us to put together the wifi NICs we want
  WifiHelper wifi;
  
  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();

  // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO); 

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");

  //Modele de propagation
  wifiChannel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel");

  wifiPhy.SetChannel (wifiChannel.Create ());

  // Add a non-QoS upper mac, and disable rate control
  //NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  WifiMacHelper wifiMac;

  //Attention: commenter l'integralite des lignes correspondants aux autres questions.  
  //Question 1
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager","DataMode",StringValue("OfdmRate54Mbps"));

  //Question 2a 
  //wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager","DataMode",StringValue("OfdmRate6Mbps"));

  //Question 2b
  //wifi.SetStandard (WIFI_PHY_STANDARD_80211g);
  //wifi.SetRemoteStationManager ("ns3::IdealWifiManager");

  //Question 2c
  //wifi.SetStandard (WIFI_PHY_STANDARD_80211n_2_4GHZ);
  //wifi.SetRemoteStationManager ("ns3::IdealWifiManager");

  // Set it to adhoc mode
  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer devicesAP;
  NetDeviceContainer devices;

  devicesAP = wifi.Install (wifiPhy, wifiMac, nodesAP);
  devices = wifi.Install (wifiPhy, wifiMac, nodes);

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  //Les STAs sont tous au meme endroit: mais ce n'est pas grave...
  for(int i=0; i<n; i++) positionAlloc->Add (Vector (distance, 0.0, 0.0));

  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes);
  mobility.Install (nodesAP);

  InternetStackHelper internet;
  internet.Install (nodes);
  internet.Install (nodesAP);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer apInterfaces = ipv4.Assign (devicesAP);
  Ipv4InterfaceContainer adhocInterfaces = ipv4.Assign (devices);

  //Serveur
  uint16_t port = 4000; // Set port.
  UdpServerHelper server (port);
  ApplicationContainer apps = server.Install (nodesAP.Get(0));
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (3.0));

  //Config du client - autant de clients que de routes (le client est la source)
  UdpClientHelper client1 (Ipv4Address("10.1.1.1"), port);//addr dest + no port
  client1.SetAttribute ("MaxPackets", UintegerValue (400000));
  client1.SetAttribute ("Interval", TimeValue(Seconds(interval)));
  client1.SetAttribute ("PacketSize", UintegerValue (packetSize));

  //On installe le client sur toutes les stations
  apps = client1.Install (nodes);
  apps.Start (Seconds (1.01));
  apps.Stop (Seconds (3.0));

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}

