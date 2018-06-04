import java.io.*;
import java.util.*;
import javax.servlet.*;
import javax.servlet.http.*;

class Kaboom extends HttpServlet {
    void causeCrash() {
	PrintWriter out = null;
	out.println("crash");
    }

    void intermediateFunction() {
	causeCrash();
    }

    @Override
    public void doGet(HttpServletRequest request, HttpServletResponse response)
                throws IOException, ServletException {

	intermediateFunction();
    }
}
