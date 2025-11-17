using System;
using System.Net;
using System.Net.Sockets;

namespace LiveLink.Abstractions;

public class LiveLinkConnection : IDisposable
{
    private Socket? _socket = null;

    public void Dispose()
    {
        GC.SuppressFinalize(this);
        _socket?.Dispose();
        _socket = null;
    }

    public bool Connect()
    {
        _socket?.Dispose();

        try
        {
            IPHostEntry ipHost = Dns.GetHostEntry(Dns.GetHostName());
            IPAddress ipAddr = ipHost.AddressList[0];
            IPEndPoint localEndPoint = new(ipAddr, 3459);

            // Creation TCP/IP Socket using 
            // Socket Class Constructor
            _socket = new(ipAddr.AddressFamily, SocketType.Stream, ProtocolType.Tcp);
            _socket.Connect(localEndPoint);
            return true;
        }
        catch
        {
            return false;
        }
    }

    public int Send(byte[] data)
    {
        try
        {
            int byteSent = _socket?.Send(data) ?? 0;
            return byteSent;
        }
        catch
        {
            return 0;
        }

    }
}
