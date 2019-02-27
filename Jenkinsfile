pipeline {
    agent none
    stages {
        stage('checkout') {
            agent any
            steps {
                echo '##################################### Checkout COMPLETED ####################################'
            }
        }
        stage('build') {
            agent any
            options { skipDefaultCheckout(true) }
            steps {
                echo '########################################## Building #########################################'
                sh '''#!/bin/bash
                        rm -rf clang_analysis
                        scan-build -o clang_analysis ./build.sh
                        RESULT=1
                        if [ -d clang_analysis ] && [ -z `ls clang_analysis/` ]; then RESULT=0; fi
                        exit $RESULT
                    '''
            }
        }
    }
}
