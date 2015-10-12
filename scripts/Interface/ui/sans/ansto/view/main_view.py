import abc

class MainView(object):

    __metaclass__ = abc.ABCMeta

    @abc.abstractmethod
    def get_run_table(self):
        pass

    @abc.abstractmethod
    def set_processing(self):
        pass
