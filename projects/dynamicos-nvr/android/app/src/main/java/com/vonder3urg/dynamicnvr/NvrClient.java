package com.vonder3urg.dynamicnvr;

import android.content.Context;
import org.json.JSONArray;
import org.json.JSONObject;
import okhttp3.*;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.cert.CertificateException;
import java.security.cert.X509Certificate;
import java.util.Locale;
import java.util.concurrent.TimeUnit;
import javax.net.ssl.SSLContext;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;
import androidx.media3.datasource.okhttp.OkHttpDataSource;

/** Certificate-pinned, bearer-authenticated NVR API. No credentials travel over plaintext RTSP. */
public final class NvrClient {
 public interface Callback { void done(String result, String error); }
 private final Context context;
 private final SecureStore store;
 private OkHttpClient client;
 private String origin, token, fingerprint;

 public NvrClient(Context c) {
  context=c;store=new SecureStore(c);
  try {
   JSONObject j=new JSONObject(store.load());
   origin=j.optString("origin");token=j.optString("token");fingerprint=j.optString("fingerprint");
   if(!origin.isEmpty()&&!fingerprint.isEmpty()) buildClient();
  }catch(Exception ignored){}
 }

 public boolean connected(){return client!=null && token!=null && !token.isEmpty();}
 public String origin(){return origin;}
 public void configure(String url,String sha256) throws Exception {
  if(!url.matches("https://(\\[[0-9a-fA-F:]+\\]|[A-Za-z0-9.-]+)(:[0-9]{2,5})?"))
   throw new IllegalArgumentException("HTTPS-Adresse erforderlich (ohne Pfad)");
  String clean=sha256.replace(":", "").replaceAll("\\s+", "").toUpperCase(Locale.ROOT);
  if(!clean.matches("[0-9A-F]{64}"))throw new IllegalArgumentException("Zertifikat-SHA256 benötigt 64 Hex-Zeichen");
  origin=url;fingerprint=clean;token="";buildClient();save();
 }
 public void save() throws Exception {JSONObject j=new JSONObject();j.put("origin",origin);j.put("fingerprint",fingerprint);j.put("token",token==null?"":token);store.save(j.toString());}

 private void buildClient() throws Exception {
  final String expected=fingerprint;
  X509TrustManager tm=new X509TrustManager(){
   @Override public void checkClientTrusted(X509Certificate[] c,String auth) throws CertificateException {throw new CertificateException("Kein Client-Zertifikat");}
   @Override public void checkServerTrusted(X509Certificate[] chain,String auth) throws CertificateException {
    if(chain==null||chain.length==0)throw new CertificateException("Zertifikat fehlt");
    try {
     byte[] digest=MessageDigest.getInstance("SHA-256").digest(chain[0].getEncoded());
     byte[] expectedBytes=new byte[32];
     for(int i=0;i<32;i++)expectedBytes[i]=(byte)Integer.parseInt(expected.substring(i*2,i*2+2),16);
     if(!MessageDigest.isEqual(digest,expectedBytes))throw new CertificateException("Zertifikats-Fingerprint stimmt nicht überein");
     chain[0].checkValidity();
    }catch(CertificateException e){throw e;}catch(Exception e){throw new CertificateException(e);}
   }
   @Override public X509Certificate[] getAcceptedIssuers(){return new X509Certificate[0];}
  };
  SSLContext ssl=SSLContext.getInstance("TLS");ssl.init(null,new TrustManager[]{tm},null);
  // Strict SHA-256 fingerprint above pins the exact self-signed certificate.
  // Normal HTTPS hostname validation remains mandatory; the pinned certificate must also match.
  client=new OkHttpClient.Builder().sslSocketFactory(ssl.getSocketFactory(),tm)
   .connectTimeout(6,TimeUnit.SECONDS).readTimeout(30,TimeUnit.SECONDS)
   .followRedirects(false).followSslRedirects(false).build();
 }
 public OkHttpDataSource.Factory mediaFactory(){
  OkHttpDataSource.Factory f=new OkHttpDataSource.Factory(client);
  if(token!=null&&!token.isEmpty())f.setDefaultRequestProperties(java.util.Collections.singletonMap("Authorization","Bearer "+token));
  return f;
 }
 public String mediaUrl(String path){return origin+path;}
 public void pair(String code,String name,Callback cb){
  try{JSONObject payload=new JSONObject().put("code",code).put("name",name);request("POST","/api/v1/pair",payload.toString(),(data,err)->{
   if(err==null){try{token=new JSONObject(data).getString("token");save();}catch(Exception e){err="Kopplung konnte nicht gespeichert werden";}}
   cb.done(data,err);
  },false);}catch(Exception e){cb.done(null,e.getMessage());}
 }
 public void get(String path,Callback cb){request("GET",path,null,cb,true);}
 public interface ImageCallback { void done(byte[] bytes, String error); }
 public void snapshot(String cameraId, ImageCallback callback){
  if(client==null || !connected()){callback.done(null,"Nicht gekoppelt");return;}
  if(!cameraId.matches("[A-Za-z0-9_-]{1,80}")){callback.done(null,"Ungültige Kamera-ID");return;}
  Request r=new Request.Builder().url(origin+"/api/v1/cameras/"+cameraId+"/snapshot.jpg")
    .header("Authorization","Bearer "+token).build();
  client.newCall(r).enqueue(new okhttp3.Callback(){
   public void onFailure(Call call,IOException e){callback.done(null,e.getMessage());}
   public void onResponse(Call call,Response response){try(Response r=response){
    if(!r.isSuccessful() || r.body()==null){callback.done(null,"Kamera nicht erreichbar");return;}
    byte[] img=r.body().bytes();
    if(img.length>4_000_000){callback.done(null,"Bild zu groß");return;}
    callback.done(img,null);
   }catch(Exception e){callback.done(null,e.getMessage());}}
  });
 }

 public void unlink(){token="";try{save();}catch(Exception ignored){}}
 private void request(String method,String path,String json,Callback cb,boolean bearer){
  if(client==null){cb.done(null,"Server noch nicht eingerichtet");return;}
  Request.Builder b=new Request.Builder().url(origin+path).header("Accept","application/json");
  if(bearer&&token!=null&&!token.isEmpty())b.header("Authorization","Bearer "+token);
  if("POST".equals(method))b.post(RequestBody.create(json,MediaType.parse("application/json")));
  else b.get();
  client.newCall(b.build()).enqueue(new CallbackAdapter(cb));
 }
 private static class CallbackAdapter implements okhttp3.Callback {
  private final Callback cb;CallbackAdapter(Callback cb){this.cb=cb;}
  public void onFailure(Call call,IOException e){cb.done(null,"Verbindung: "+e.getMessage());}
  public void onResponse(Call call,Response r){try(Response body=r){
   String s=r.body()==null?"":r.body().string();
   if(r.isSuccessful())cb.done(s,null);else cb.done(null,"HTTP "+r.code()+": "+s);
  }catch(Exception e){cb.done(null,e.getMessage());}}
 }
}
