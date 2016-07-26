import urllib.request
import json

def getPrediction(width,height,area):
    data =  {

            "Inputs": {

                    "input1":
                    {
                        "ColumnNames": ["Width", "Height", "Area", "Gesture"],
                        "Values": [ [ width, height, area, "5" ] ]
                    },        },
                "GlobalParameters": {
    }
        }

    body = str.encode(json.dumps(data))

    url = 'https://ussouthcentral.services.azureml.net/workspaces/02c3fdb18e1044a8891eabc3517b8d30/services/84dc826d9eee4468b9374c15ed6453ac/execute?api-version=2.0&details=true'
    api_key = 'Qi51zMzbBDwM/aO/W9w/eaGTg6TauH4b+Kk02UYxYLrfO9yN0QGZtyC0AF5htoAgdIzpMNJRDlF+e5lyJo2oYA==' # Replace this with the API key for the web service
    headers = {'Content-Type':'application/json', 'Authorization':('Bearer '+ api_key)}
    req = urllib.request.Request(url, body, headers)
    try:
        response = urllib.request.urlopen(req)

        # If you are using Python 3+, replace urllib2 with urllib.request in the above code:
        # req = urllib.request.Request(url, body, headers) 
        # response = urllib.request.urlopen(req)

        result = response.read()
        prediction= json.loads(result.decode())
        #print(getPrediction(json))
        t1 = prediction['Results']
        t2 = t1['output1']
        t3 = t2['value']
        t4 = t3['Values']
        return t4[0][7]
        
    except urllib.HTTPError.error:
        print("The request failed with status code: " + str(error.code))
        return 5

