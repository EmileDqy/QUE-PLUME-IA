"""
Created on Fri Mar 26 15:03:21 2021

@author: Lucie
"""

import os
try:
  import pyrebase
except ImportError:
  os.system('python3 -m pip install pyrebase')
import pyrebase
import argparse
import datetime

config = {
  "apiKey": "AAAA_8U_Wc0:APA91bHGpXr9EqK0gsk25OkcdO5cVV3I7b83dj0dIS4ri1X__HxC86UhRXnHeTbdwZ6BINuLwJ7zf0lnDaau4toJ7EitnMFnna1ZaXGJWivVg3w3H7_d8zwe7UwWBlqrXvLZiEuX5zPl",
  "authDomain": "projetia-d96bc.firebaseapp.com",
  "databaseURL": "https://projetia-d96bc-default-rtdb.firebaseio.com",
  "storageBucket": "projetia-d96bc.appspot.com"
}

if __name__ == "__main__":
  #argparser
  my_parser = argparse.ArgumentParser(description='Push data to firebase')
  my_parser.add_argument('type_expected',
                        help='type expected of the object')
  my_parser.add_argument('color_expected',
                        help='color expected of the object')
  my_parser.add_argument('type_found',
                        help='type estimated for the object')
  my_parser.add_argument('color_found',
                        help='color estimated for the object')

  # Setup firebase
  firebase = pyrebase.initialize_app(config)
  db = firebase.database()
  
  # Vars
  dateheure = str(datetime.datetime.now())

  # Execute parse_args()
  args = my_parser.parse_args()
  data = {"type_reel":args.type_expected,"couleur_reelle":args.color_expected,"date":dateheure,"couleur_trouvee" : args.color_found,"id":"1","type_trouve" :args.type_found}
  db.child("erreurs").push(data)
  print('The data has been pushed')





