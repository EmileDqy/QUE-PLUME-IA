"""
Created on Fri Mar 26 15:03:21 2021

@author: Lucie
"""

import pyrebase
import argparse
import datetime

config = {
  "apiKey": "AAAA_8U_Wc0:APA91bHGpXr9EqK0gsk25OkcdO5cVV3I7b83dj0dIS4ri1X__HxC86UhRXnHeTbdwZ6BINuLwJ7zf0lnDaau4toJ7EitnMFnna1ZaXGJWivVg3w3H7_d8zwe7UwWBlqrXvLZiEuX5zPl",
  "authDomain": "projetia-d96bc.firebaseapp.com",
  "databaseURL": "https://projetia-d96bc-default-rtdb.firebaseio.com",
  "storageBucket": "projetia-d96bc.appspot.com"
}

if __main__ == "__name__":
  #argparser
  my_parser = argparse.ArgumentParser(description='Push data to firebase')

  my_parser.add_argument('type',
                        help='type of the object')
  my_parser.add_argument('color',
                        help='color of the object')

  # Setup firebase
  firebase = pyrebase.initialize_app(config)
  db = firebase.database()
  
  dateheure = str(datetime.datetime.now())

  # Execute parse_args()
  args = my_parser.parse_args()
  data = {"type_reel":args.type,"couleur_reelle":args.color,"date":dateheure,"couleur_trouvee" :"noire","id":"1","type_trouve" :"couvercle"}
  db.child("erreurs").push(data)
  print('The data has been pushed')





