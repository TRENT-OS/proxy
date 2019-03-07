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
            agent {
                docker {
                    image 'camkes_build_env_20190307'
                    // bind the localtime to avoid problems of gaps between the localtime of the container and the host
                    args '-v /etc/localtime:/etc/localtime:ro'
                }
            }
            options { skipDefaultCheckout(true) }
            steps {
                echo '########################################## Building #########################################'
                sh '''#!/bin/bash
                        # install needed dependecies
                        apt-get install libvdeplug2-dev vde2

                        rm -rf clang_analysis
                        scan-build -o clang_analysis ./build.sh
                        RESULT=1
                        if [ -d clang_analysis ] && [ -z `ls clang_analysis/` ]; then RESULT=0; else rm -rf build; fi
                        exit $RESULT
                    '''
                cleanWs()
            }
        }
    }
}
