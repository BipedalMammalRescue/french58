// A C# program for Client
using LiveLink.Abstractions;

using LiveLinkConnection connection = new();
if (!connection.Connect())
{
    Console.WriteLine("Failed to connect to game.");
    Environment.Exit(1);    
}

byte[] messageSent = [(byte)PacketType.Ping];
int byteSent = connection.Send(messageSent);