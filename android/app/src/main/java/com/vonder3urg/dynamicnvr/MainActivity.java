package com.vonder3urg.dynamicnvr;

import android.app.Activity;
import android.app.AlertDialog;
import android.graphics.Color;
import android.graphics.BitmapFactory;
import android.graphics.Bitmap;
import android.widget.ImageView;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.os.Bundle;
import android.view.WindowManager;
import android.view.View;
import android.widget.*;
import androidx.media3.common.MediaItem;
import androidx.media3.common.PlaybackException;
import androidx.media3.common.Player;
import androidx.media3.exoplayer.ExoPlayer;
import androidx.media3.exoplayer.hls.HlsMediaSource;
import androidx.media3.exoplayer.source.ProgressiveMediaSource;
import androidx.media3.ui.PlayerView;
import org.json.JSONArray;
import org.json.JSONObject;
import java.net.URLEncoder;

/** Native remote console. Camera credentials and raw RTSP are never stored on Android. */
public final class MainActivity extends Activity {
 private static final int BG=0xff080f1b, CARD=0xff142339, BORDER=0xff25445d, AQUA=0xff60d7e7, TEXT=0xfff1f6ff, MUTED=0xff9bb2c9;
 private NvrClient api;
 private LinearLayout root, cameraList, recordingList;
 private PlayerView playerView;
 private TextView status, section;
 private ExoPlayer player;
 private JSONArray cameras=new JSONArray(), recordings=new JSONArray();
 private TextView overview;
 private String selectedCamera="";
 private int playbackRetries=0;
 @Override public void onCreate(Bundle state){super.onCreate(state);getWindow().addFlags(WindowManager.LayoutParams.FLAG_SECURE);getWindow().setStatusBarColor(BG);getWindow().setNavigationBarColor(BG);
  api=new NvrClient(this);render();if(api.connected())refresh();}
 private int dp(float x){return (int)(x*getResources().getDisplayMetrics().density+.5f);}
 private TextView text(String s,int size,int color,boolean bold){TextView v=new TextView(this);v.setText(s);v.setTextSize(size);v.setTextColor(color);if(bold)v.setTypeface(Typeface.DEFAULT,Typeface.BOLD);return v;}
 private GradientDrawable surface(int color){GradientDrawable d=new GradientDrawable();d.setColor(color);d.setCornerRadius(dp(16));d.setStroke(dp(1),BORDER);return d;}
 private TextView action(String s,Runnable callback){TextView v=text(s,13,TEXT,true);v.setGravity(17);v.setPadding(dp(12),dp(12),dp(12),dp(12));v.setBackground(surface(CARD));v.setOnClickListener(w->callback.run());return v;}
 private void rowButton(LinearLayout row,String title,Runnable fn){TextView b=action(title,fn);LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(0,-2,1);lp.setMargins(dp(3),0,dp(3),0);row.addView(b,lp);}
 private void render(){ScrollView scroll=new ScrollView(this);scroll.setFillViewport(true);root=new LinearLayout(this);root.setPadding(dp(18),dp(22),dp(18),dp(28));root.setOrientation(1);root.setBackgroundColor(BG);scroll.addView(root);setContentView(scroll);
  TextView wordmark=text("dynamicOS",28,TEXT,true);root.addView(wordmark);
  TextView tag=text("/ NVR     ·     SECURE OPERATIONS",11,AQUA,true);tag.setPadding(0,0,0,dp(18));root.addView(tag);
  TextView heading=text("Sicherheitszentrale",21,TEXT,true);heading.setPadding(0,0,0,dp(12));root.addView(heading);
  overview=text("SYSTEMSTATUS  ·  SERVER NICHT VERBUNDEN",12,AQUA,true);
  overview.setPadding(dp(14),dp(16),dp(14),dp(16));overview.setBackground(surface(CARD));
  LinearLayout.LayoutParams ovLp=new LinearLayout.LayoutParams(-1,-2);ovLp.bottomMargin=dp(14);root.addView(overview,ovLp);
  playerView=new PlayerView(this);playerView.setUseController(true);playerView.setBackgroundColor(Color.BLACK);playerView.setBackground(surface(CARD));
  root.addView(playerView,new LinearLayout.LayoutParams(-1,dp(238)));
  status=text(api.connected()?"Verbunden · Tippe auf eine Kamera":"Nicht gekoppelt · Server einrichten",12,MUTED,false);
  status.setPadding(0,dp(12),0,dp(18));root.addView(status);
  LinearLayout actions=new LinearLayout(this);actions.setOrientation(0);root.addView(actions);
  rowButton(actions,"Server koppeln",this::setup);rowButton(actions,"Aktualisieren",this::refresh);rowButton(actions,"Ereignisse",this::events);
  label("LIVE-KAMERAS");cameraList=new LinearLayout(this);cameraList.setOrientation(1);root.addView(cameraList);
  label("AUFZEICHNUNGEN");recordingList=new LinearLayout(this);recordingList.setOrientation(1);root.addView(recordingList);
  TextView note=text("Nur über HTTPS mit Zertifikatsbindung · Kein ZOSI-Cloudzugang",11,MUTED,false);note.setPadding(0,dp(25),0,0);root.addView(note);
 }
 private void label(String s){TextView v=text(s,11,AQUA,true);v.setPadding(0,dp(24),0,dp(10));root.addView(v);}
 private void notice(String s){runOnUiThread(()->status.setText(s));}
 private void setup(){LinearLayout fields=new LinearLayout(this);fields.setOrientation(1);fields.setPadding(dp(14),dp(5),dp(14),0);
  EditText url=input("Mac mini: https://192.168.x.x:8443",api.origin());
  EditText fp=input("Zertifikat SHA-256 (64 Hex-Zeichen)","");
  EditText code=input("Einmaliger Kopplungscode","");
  EditText name=input("Gerätename","Android NVR");
  for(EditText e:new EditText[]{url,fp,code,name})fields.addView(e);
  new AlertDialog.Builder(this).setTitle("Sichere Kopplung").setMessage("Am Mac den SHA-256-Fingerprint vergleichen und einen neuen Kopplungscode erzeugen.").setView(fields)
   .setPositiveButton("Koppeln",(d,w)->{
    try{api.configure(url.getText().toString().trim(),fp.getText().toString());
     notice("Verschlüsselte Kopplung …");api.pair(code.getText().toString().trim(),name.getText().toString().trim(),(data,error)->{
      if(error!=null)notice("Kopplung: "+error);else{notice("Sicher verbunden");refresh();}
     });
    }catch(Exception ex){notice(ex.getMessage());}
   }).setNegativeButton("Abbrechen",null).show();
 }
 private EditText input(String hint,String value){EditText e=new EditText(this);e.setTextColor(TEXT);e.setHintTextColor(MUTED);e.setTextSize(14);e.setHint(hint);e.setSingleLine(true);e.setText(value);return e;}
 private void refresh(){if(!api.connected()){notice("Zuerst sicheren Server koppeln");return;}
  api.get("/api/v1/status",(json,error)->{
   if(error==null)try{JSONObject j=new JSONObject(json);runOnUiThread(()->overview.setText(
     "GESICHERT  ·  "+j.optInt("cameras")+" KAMERAS  ·  "+j.optInt("active_recordings")+" AUFNAHMEN  ·  HTTPS\n24/7: "+j.optString("record_mode")+"  ·  Aufbewahrung: "+j.optInt("retention_hours")+" h"));}
   catch(Exception ignored){}
  });
  api.get("/api/v1/cameras",(json,error)->{if(error!=null){notice(error);return;}try{cameras=new JSONArray(json);runOnUiThread(this::updateCameras);}catch(Exception e){notice("Kameraliste ungültig");}});
  api.get("/api/v1/recordings",(json,error)->{if(error!=null){notice(error);return;}try{recordings=new JSONArray(json);runOnUiThread(this::updateRecordings);}catch(Exception e){notice("Aufzeichnungsliste ungültig");}});
 }
 private void updateCameras(){cameraList.removeAllViews();if(cameras.length()==0){cameraList.addView(text("Keine Kameras am Mac eingerichtet",13,MUTED,false));return;}
  for(int i=0;i<cameras.length();i++){JSONObject c=cameras.optJSONObject(i);if(c==null)continue;
   String id=c.optString("id"),name=c.optString("name");
   LinearLayout card=new LinearLayout(this);card.setOrientation(1);card.setPadding(dp(10),dp(10),dp(10),dp(10));card.setBackground(surface(CARD));
   ImageView preview=new ImageView(this);preview.setScaleType(ImageView.ScaleType.CENTER_CROP);preview.setBackgroundColor(0xff0c1725);
   card.addView(preview,new LinearLayout.LayoutParams(-1,dp(150)));
   TextView item=action("●  "+name+"     LIVE ÖFFNEN",()->playLive(id,name));
   LinearLayout.LayoutParams actionLp=new LinearLayout.LayoutParams(-1,-2);actionLp.topMargin=dp(8);card.addView(item,actionLp);
   LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(-1,-2);lp.setMargins(0,0,0,dp(12));cameraList.addView(card,lp);
   api.snapshot(id,(bytes,error)->{if(bytes!=null){Bitmap b=BitmapFactory.decodeByteArray(bytes,0,bytes.length);
     if(b!=null)runOnUiThread(()->preview.setImageBitmap(b));}});}
 }
 private void updateRecordings(){recordingList.removeAllViews();if(recordings.length()==0){recordingList.addView(text("Noch keine Aufzeichnungen",13,MUTED,false));return;}
  for(int i=0;i<Math.min(100,recordings.length());i++){JSONObject r=recordings.optJSONObject(i);if(r==null)continue;
   String id=r.optString("id"),name=r.optString("camera_name"),date=r.optString("timestamp");
   TextView item=action("▣   "+name+"  ·  "+(date.length()>16?date.substring(0,16):date),()->playRecording(id));
   LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(-1,-2);lp.setMargins(0,0,0,dp(8));recordingList.addView(item,lp);}
 }
 private void playLive(String id,String label){selectedCamera=id;String path="/api/v1/cameras/"+id+"/live/index.m3u8";
  // First request starts FFmpeg relay. Retry once while the initial HLS segments appear.
  api.get(path,(r,e)->{
   if(e!=null && !e.startsWith("HTTP 503")){notice(e);return;}
   runOnUiThread(()->{
    startPlayer(true,path);notice("LIVE · "+label+" · HTTPS");
   });
  });
 }
 private void playRecording(String id){try{
  String encoded=java.net.URLEncoder.encode(id,"UTF-8").replace("+","%20");
  startPlayer(false,"/api/v1/recordings/"+encoded+"/play.mp4");notice("Wiedergabe · verschlüsselte Verbindung");
 }catch(Exception ex){notice(ex.getMessage());}}
 private void startPlayer(boolean hls,String path){if(player!=null){player.release();player=null;}
  playbackRetries=0;
  player=new ExoPlayer.Builder(this).build();playerView.setPlayer(player);
  MediaItem item=MediaItem.fromUri(api.mediaUrl(path));
  if(hls)player.setMediaSource(new HlsMediaSource.Factory(api.mediaFactory()).createMediaSource(item));
  else player.setMediaSource(new ProgressiveMediaSource.Factory(api.mediaFactory()).createMediaSource(item));
  player.addListener(new Player.Listener(){
   @Override public void onPlaybackStateChanged(int state){if(state==Player.STATE_READY)playbackRetries=0;}
   @Override public void onPlayerError(PlaybackException e){
    if(hls && playbackRetries++<6){notice("Kamerastream wird neu verbunden …");
     new android.os.Handler(android.os.Looper.getMainLooper()).postDelayed(()->{
      if(player!=null){player.prepare();player.play();}
     },2000);
    }else notice("Wiedergabefehler: "+e.getErrorCodeName());
   }
  });
  player.prepare();player.play();
 }
 private void events(){if(!api.connected()){notice("Nicht verbunden");return;}
  api.get("/api/v1/events",(json,err)->{if(err!=null){notice(err);return;}
   try{JSONArray list=new JSONArray(json);StringBuilder b=new StringBuilder();for(int i=0;i<Math.min(30,list.length());i++){
    JSONObject e=list.getJSONObject(i);b.append(e.optString("at")).append("\n").append(e.optString("camera")).append(" · ").append(e.optString("type")).append("\n\n");}
    runOnUiThread(()->new AlertDialog.Builder(this).setTitle("Sicherheitsereignisse").setMessage(b.length()==0?"Keine Ereignisse":b.toString()).setPositiveButton("Schließen",null).show());
   }catch(Exception e){notice("Ereignisliste nicht lesbar");}
  });
 }
 @Override protected void onStop(){super.onStop();if(player!=null)player.pause();}
 @Override protected void onDestroy(){if(player!=null)player.release();super.onDestroy();}
}
