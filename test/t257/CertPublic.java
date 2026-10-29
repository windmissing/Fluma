import java.nio.file.*;
import java.security.*;
import java.security.cert.*;
// Public certificate SPKI fingerprints only; never reads a private key or password.
public class CertPublic {
 public static void main(String[] args)throws Exception {
  for(String path:args){
   var cert=(X509Certificate)CertificateFactory.getInstance("X.509").generateCertificate(Files.newInputStream(Path.of(path)));
   System.out.println(java.util.HexFormat.of().formatHex(MessageDigest.getInstance("SHA-256").digest(cert.getPublicKey().getEncoded())));
  }
 }
}
