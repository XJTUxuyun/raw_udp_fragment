public class UdpWithRaw
{
	static {
		System.loadLibrary("UdpWithRawNative");
	}
	public native int createRawSock();
	public native int closeRawSock(int fd);
	public native int sentWithMtu(int fd, byte[] buf, String src, int src_port, String dst, int dst_port);

	public static void main(String[] args)
	{
		System.out.println("hello world");
		UdpWithRaw udp = new UdpWithRaw();
		int fd = udp.createRawSock();
		System.out.println(fd);
		byte[] buf = new byte[1024];
		buf[0] = 'a';
		System.out.println(udp.sentWithMtu(fd, buf, "192.168.66.128", 7777, "232.1.1.1", 8888));
		System.out.println(udp.closeRawSock(fd));
	}
}
