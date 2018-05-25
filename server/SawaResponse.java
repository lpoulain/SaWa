import java.lang.String;
import java.util.*;
import java.io.*;
import javax.servlet.*;
import javax.servlet.http.*;

class SawaResponse implements HttpServletResponse {
    ByteArrayOutputStream stream;
    PrintWriter writer;
    String output;

    public SawaResponse() {
        stream = new ByteArrayOutputStream();
        writer = new PrintWriter(stream);
    }

    public PrintWriter getWriter() { return writer; }

    public byte[] getOutput() {
        System.out.println("Closing...");
        writer.flush();
        return stream.toByteArray();
    }

    public void addCookie(Cookie cookie) { }
    public void addDateHeader(String name, long date) { }
    public void addHeader(String name, String value) { }
    public void addIntHeader(String name, int value) { }
    public boolean containsHeader(String name) { return false; }
    public String encodeRedirectUrl(String url) { return url; }
    public String encodeRedirectURL(String url) { return url; }
    public String encodeUrl(String url) { return url; }
    public String encodeURL(String url) { return url; }
    public void flushBuffer() { }
    public int getBufferSize() { return 0; }
    public String getCharacterEncoding() { return ""; }
    public String getContentType() { return ""; }
    public String getHeader(String name) { return ""; }
    public Collection<String> getHeaderNames() { return null; }
    public Collection<String> getHeaders(String name) { return null; }
    public Locale getLocale() { return null; }
    public int getStatus() { return 0; }
    public ServletOutputStream getOutputStream() { return null; }
    public boolean isCommitted() { return false; }
    public void reset() { }
    public void resetBuffer() { }
    public void sendError(int sc) { }
    public void sendError(int sc, String msg) { }
    public void sendRedirect(String location) { }
    public void setBufferSize(int size) { }
    public void setCharacterEncoding(String enc) { }
    public void setContentLength(int length) { }
    public void setContentType(String contentType) { }
    public void setDateHeader(String name, long data) { }
    public void setHeader(String name, String value) { }
    public void setIntHeader(String name, int value) { }
    public void setLocale(Locale l) { }
    public void setStatus(int sc) { }
    public void setStatus(int sc, String sm) { }
}
