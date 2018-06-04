import java.lang.String;
import java.lang.StringBuffer;
import java.security.Principal;
import java.util.*;
import java.io.*;
import javax.servlet.*;
import javax.servlet.http.*;

class TowaRequest implements HttpServletRequest {
    String method;
    String queryString;
    HashMap<String, String[]> parameters;

    public TowaRequest(String method, String queryString) {
	this.method = method;
	this.queryString = queryString;

	parameters = new HashMap<String, String[]>();

	for (String param : queryString.split("&")) {
	    String[] values = param.split("=");
	    if (values.length >= 2)
		parameters.put(values[0], new String[] { values[1] });
	}
    }

    public String getParameter(String name) {
	if (!parameters.containsKey(name)) return null;
	return parameters.get(name)[0];
    }

    public Enumeration<String> getParameterNames() {
	return Collections.enumeration(parameters.keySet());
    }

    public String[] getParameterValues(String name) {
	if (!parameters.containsKey(name)) return null;
	return parameters.get(name);
    }
    
    public Map<String, String[]> getParameterMap() { return parameters; }

    public boolean authenticate(HttpServletResponse response) { return true; }
    public String getAuthType() { return ""; }
    public AsyncContext getAsyncContext() { return null; }
    public String getContextPath() { return ""; }
    public Cookie[] getCookies() { return new Cookie[0]; }
    public long getDateHeader(String name) { return 0; }
    public DispatcherType getDispatcherType() { return null; }
    public String getHeader(String name) { return ""; }
    public Enumeration<String> getHeaderNames() { return Collections.enumeration(new ArrayList<String>()); }
    public Enumeration<String> getHeaders(String name) { return Collections.enumeration(new ArrayList<String>()); }
    public int getIntHeader(String name) { return 0; }
    public String getMethod() { return method; }
    public Part getPart(java.lang.String name) { return null; }
    public Collection<Part> getParts() { return new ArrayList<Part>(); }
    public String getPathInfo() { return ""; }
    public String getPathTranslated() { return ""; }
    public String getQueryString() { return queryString; }
    public String getRemoteUser() { return null; }
    public String getRequestedSessionId() { return null; }
    public String getRequestURI() { return null; }
    public StringBuffer getRequestURL() { return null; }
    public String getServletPath() { return null; }
    public HttpSession getSession() { return null; }
    public HttpSession getSession(boolean create) { return null; }
    public Principal getUserPrincipal() { return null; }
    public boolean isAsyncStarted() { return false; }
    public boolean isAsyncSupported() { return false; }
    public boolean isRequestedSessionIdFromCookie() { return false; }
    public boolean isRequestedSessionIdFromUrl() { return false; }
    public boolean isRequestedSessionIdFromURL() { return false; }
    public boolean isRequestedSessionIdValid() { return false; }
    public boolean isUserInRole(String role) { return false; }
    public void	login(String username, String password) { }
    public void logout() { }

    public Object getAttribute(String name) { return null; }
    public Enumeration<String> getAttributeNames() { return Collections.enumeration(new ArrayList<String>()); }
    public String getCharacterEncoding() { return null; }
    public void setCharacterEncoding(java.lang.String env) { }
    public int getContentLength() { return 0; }
    public String getContentType() { return null; }
    public ServletInputStream getInputStream() { return null; }
    public String getProtocol() { return "HTTP/1.1"; }
    public String getScheme() { return "http"; }
    public String getServerName() { return "localhost"; }
    public int getServerPort() { return 8080; }
    public BufferedReader getReader() { return null; }
    public String getRemoteAddr() { return ""; }
    public java.lang.String getRemoteHost() { return ""; }
    public void setAttribute(String name, Object o) { }
    public void removeAttribute(String name) { }
    public Locale getLocale() { return null; }
    public Enumeration<Locale> getLocales() { return Collections.enumeration(new ArrayList<Locale>()); }
    public boolean isSecure() { return false; }
    public RequestDispatcher getRequestDispatcher(String path) { return null; }
    public String getRealPath(String path) { return ""; }
    public int getRemotePort() { return 0; }
    public String getLocalName() { return "localhost"; }
    public String getLocalAddr() { return "127.0.0.1"; }
    public int getLocalPort() { return 8080; }
    public ServletContext getServletContext() { return null; }
    public AsyncContext startAsync() { return null; }
    public AsyncContext startAsync(ServletRequest servletRequest, ServletResponse servletResponse) { return null; }
}
