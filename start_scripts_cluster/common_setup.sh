TOOL_NAME=`echo $START_SCRIPT | sed 's!.*_!!' | sed 's!\..*!!' `

CONFERENCE="esa15"

EXPERIMENT_NAME="$CONFERENCE.$TOOL_NAME"
GRAPH_NAME=`echo $INSTANCE | sed 's!.*/!!'`

chmod +x "$START_SCRIPT"

if [[ ! -d "$HOME/experiments/$CONFERENCE/results" ]]; then
    echo "experiments folder not found in $HOME"
    echo "creating it..."
    cd "$HOME/experiments"
    if [[ ! -d "$HOME/experiments/$CONFERENCE" ]]; then
	mkdir $CONFERENCE
    fi
    cd $CONFERENCE
    if [[ ! -d "$HOME/experiments/$CONFERENCE/results" ]]; then
	mkdir "results"
    fi
fi

cd "$HOME/experiments/$CONFERENCE/results"

if [[ ! -d "$EXPERIMENT_NAME" ]]; then
    echo "subfolder $EXPERIMENT_NAME not found"
    echo "creating it..."
    mkdir "$EXPERIMENT_NAME"
fi

cd "$EXPERIMENT_NAME"
