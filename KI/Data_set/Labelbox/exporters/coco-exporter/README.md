# COCO Exporter

Inside this folder is a script for converting Labelbox JSON to COCO file exports. To get a JSON export file from your project, 
visit the export tab of your project and select the JSON option from the dropdown. This will be the file input for the COCO converter script in this directory. 

First clone the repository to access the converter script aka:
```sh
git clone https://github.com/Labelbox/Labelbox.git
```

To get started you'll want to download and install Docker on your desktop. 
Follow the installation and setup instructions at [docker-hub](https://www.docker.com/products/docker-desktop).

Once you have installed Docker, you may run the converter inside a docker container. To do so simply run the following command from this directory. 
Note: The COCO export output location, if unmodified, will be saved in same directory as the input file. 
```sh
# e.g. if your json export file is named "export.json" and is in your Downloads directory

make run-local-export EXPORT_PATH=~/Downloads/export.json
```

Should you wish to modify the location of the export output, you can also run:
```sh
# e.g. if your json export file is named "export.json" and is in your Downloads directory 
# and you want to save the output in a separate directory, here named "exports"

make run-local-export EXPORT_PATH=~/Downloads/export.json OUTPUT_DEST=~/Desktop/exports
```

We HIGHLY recommend you use the Docker container to run the converter script `main.py`. The docker container provides an isolated environment with the correct dependencies installed, making it easier to setup than even a pipenv.

Finally should you absolutely need to run the script directly we recommend using pipenv. The script uses Python 3.6.4 and `pipenv` to manage dependencies.
                                                                              
To install dependencies, run:
```sh
pipenv install
```

To then run the converter script, run: 
```sh
pipenv python main.py ${PATH_TO_INPUT} ${PATH_TO_OUTPUT}
```
