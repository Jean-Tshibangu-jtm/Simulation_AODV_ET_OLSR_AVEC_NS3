/* 
 */

#include <fstream>
#include <iostream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/aodv-module.h"
#include "ns3/olsr-module.h"
#include "ns3/dsdv-module.h"
#include "ns3/dsr-module.h"
#include "ns3/applications-module.h"

using namespace ns3;
using namespace dsr;

NS_LOG_COMPONENT_DEFINE ("manet-routing-compare");

int
main (int argc, char *argv[])
{

  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  int nSinks = 1; 	//IFI: Nombre de source-destination
  double txp = 7.5;	//Puissance d'emision (ne pas modifier)
  int protocol=2;//OLSR=1 AODV=2 DSR=3

  CommandLine cmd;

  cmd.AddValue ("protocol", "Routing protocol", protocol);

  Packet::EnablePrinting ();

  int nWifis = 20;     		//IFI: Nombre de noeuds

  double TotalTime = 100.0;	//IFI: Temps total des simulations
  std::string phyMode ("DsssRate11Mbps");
  std::string tr_name ("trace-ifi-ns3");
  double nodeSpeed = 0.1; //in m/s	//IFI: Vitesse maximum des noeuds du RWP (ne peut pas etre nul mais peut etre tres petit)
  int nodePause = 0; //in s	//Pas de pause

  //Set Non-unicastMode rate to unicast mode
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",StringValue (phyMode));

  NodeContainer nodes;
  nodes.Create (nWifis);

  // setting up wifi phy and channel using helpers
  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
  wifiPhy.SetChannel (wifiChannel.Create ());

  // Add a non-QoS upper mac, and disable rate control
  //NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  WifiMacHelper wifiMac;

  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));

  wifiPhy.Set ("TxPowerStart",DoubleValue (txp));
  wifiPhy.Set ("TxPowerEnd", DoubleValue (txp));

  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer adhocDevices = wifi.Install (wifiPhy, wifiMac, nodes);

  MobilityHelper mobilityAdhoc;
  int64_t streamIndex = 0; // used to get consistent mobility across scenarios

  ObjectFactory pos;
  pos.SetTypeId ("ns3::RandomRectanglePositionAllocator");
  pos.Set ("X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=300.0]"));
  pos.Set ("Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1500.0]"));

  Ptr<PositionAllocator> taPositionAlloc = pos.Create ()->GetObject<PositionAllocator> ();
  streamIndex += taPositionAlloc->AssignStreams (streamIndex);

  std::stringstream ssSpeed;
  ssSpeed << "ns3::UniformRandomVariable[Min=0.0|Max=" << nodeSpeed << "]";
  std::stringstream ssPause;
  ssPause << "ns3::ConstantRandomVariable[Constant=" << nodePause << "]";
  mobilityAdhoc.SetMobilityModel ("ns3::RandomWaypointMobilityModel",
                                  "Speed", StringValue (ssSpeed.str ()),
                                  "Pause", StringValue (ssPause.str ()),
                                  "PositionAllocator", PointerValue (taPositionAlloc));
  mobilityAdhoc.SetPositionAllocator (taPositionAlloc);
  mobilityAdhoc.Install (nodes);
  streamIndex += mobilityAdhoc.AssignStreams (nodes, streamIndex);

  AodvHelper aodv;
  OlsrHelper olsr;
  DsdvHelper dsdv;
  DsrHelper dsr;
  DsrMainHelper dsrMain;
  Ipv4ListRoutingHelper list;
  InternetStackHelper internet;

  switch (protocol)
    {
    case 1:
      list.Add (olsr, 100); //OLSR
      break;
    case 2:
      list.Add (aodv, 100); //AODV
      break;
    case 3:
      break;	//DSR
    default:
      NS_FATAL_ERROR ("No such protocol:" << protocol);
    }

  if (protocol < 3)
    {
      internet.SetRoutingHelper (list);
      internet.Install (nodes);
    }
  else if (protocol == 3)
    {
      internet.Install (nodes);
      dsrMain.Install (dsr, nodes);
    }

  NS_LOG_INFO ("assigning ip address");

  Ipv4AddressHelper addressAdhoc;
  addressAdhoc.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer adhocInterfaces;
  adhocInterfaces = addressAdhoc.Assign (adhocDevices);

  /* Setting applications */
  UdpEchoServerHelper echoServer (9);
  for (int i = 0; i <= nSinks - 1; i++)
    {
  	ApplicationContainer serverApps = echoServer.Install (nodes.Get (i));
  	serverApps.Start (Seconds (59.0));
  	serverApps.Stop (Seconds (91.0));

	/* IFI: modifier les valeurs ci-dessous pour gerer le debit...automatiser peut-etre... */
  	UdpEchoClientHelper echoClient (adhocInterfaces.GetAddress (i), 9);
  	echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  	echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  	echoClient.SetAttribute ("PacketSize", UintegerValue (1500));

  	ApplicationContainer clientApps = echoClient.Install (nodes.Get (i + nSinks));
  	clientApps.Start (Seconds (60.0));
  	clientApps.Stop (Seconds (90.0));
   }

  AsciiTraceHelper ascii;
  Ptr<OutputStreamWrapper> osw = ascii.CreateFileStream ( (tr_name + ".tr").c_str());
  wifiPhy.EnableAsciiAll (osw);

  NS_LOG_INFO ("Run Simulation.");

  Simulator::Stop (Seconds (TotalTime));
  Simulator::Run ();

  Simulator::Destroy ();
}

