// A C# program for Client
using System.Net;
using System.Net.Sockets;
using LiveLink.Abstractions;


IPHostEntry ipHost = Dns.GetHostEntry(Dns.GetHostName());
IPAddress ipAddr = ipHost.AddressList[0];
IPEndPoint localEndPoint = new(ipAddr, 3459);

try
{
    // Creation TCP/IP Socket using 
    // Socket Class Constructor
    using Socket sender = new(ipAddr.AddressFamily, SocketType.Stream, ProtocolType.Tcp);

    sender.Connect(localEndPoint);
    byte[] messageSent = [(byte)PacketType.Ping];
    int byteSent = sender.Send(messageSent);
}
// Manage of Socket's Exceptions
catch (ArgumentNullException ane)
{
    Console.WriteLine("ArgumentNullException : {0}", ane.ToString());
}
catch (SocketException se)
{
    Console.WriteLine("SocketException : {0}", se.ToString());
}
catch (Exception e)
{
    Console.WriteLine("Unexpected exception : {0}", e.ToString());
}
