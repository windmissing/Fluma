import java.nio.file.*;
import java.security.cert.*;
import java.util.*;
public class VerifyChain {
 public static void main(String[] args)throws Exception {
  var factory=CertificateFactory.getInstance("X.509");
  var certificates=new ArrayList<java.security.cert.Certificate>(factory.generateCertificates(Files.newInputStream(Path.of(args[0]))));
  if(certificates.size()<2)throw new IllegalArgumentException("CHAIN_REQUIRED");
  for(int i=0;i<certificates.size();i++){
   var current=(X509Certificate)certificates.get(i);current.checkValidity();
   var issuer=(X509Certificate)certificates.get(Math.min(i+1,certificates.size()-1));current.verify(issuer.getPublicKey());
  }
  System.out.println("VALID_CHAIN");
 }
}
