import requests
import json
import pprint
pos = (50.0622614, 19.9638605)
url = 'http://busio.com.pl/Home/GetNearestStops?x={}&y={}&r=1000'.format(*pos);
json = json.loads(requests.get(url).text)
json = [tup for tup in json if 'name' in tup['data']]
positions = [(pos[0], pos[1], "JU!")] + [ (tup['x'], tup['y'], tup['data']['name']) for tup in json ]
html = """
 <html>
 <head>                                                                   
   <title></title>                                                        
   <script src="http://maps.google.com/maps/api/js?sensor=true"></script> 
   <script src="gmaps/gmaps.js"></script>                                       
   <style type="text/css">                                                
     #map {                                                               
       width: 1000px;                                                      
       height: 800px;                                                     
     }                                                                    
   </style>                                                               
 </head>                                                                  
 <body>                                                                   
   <div id="map"></div>                                                   
   <script>                                                               
     var map = new GMaps({                                                
       el: '#map',                                                        
       lat: %s,                                                   
       lng: %s                                                   
     });
     """ % pos + ''.join(["map.addMarker({lat: %s, lng: %s, title: '%s'});" % tup for tup in positions]) + """
   </script>                                                              
 </body>                                                                  
 </html>                                                                  
""" 
print html.encode('utf-8')
