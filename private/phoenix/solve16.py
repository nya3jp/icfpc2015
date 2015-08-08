import collections

DATA = (
    'alalaallaalaapaaaaaaaalaaalllalaalalaaaaalaaaalllllllllaaaappaaaallllllll'
    'llaadlaaaaadlallllaaakappppdplalllbbaalaalllllllllalbbdbalbadaaallllllapp'
    'kaapppdaallllaaallallalaalaalaaadalaallllllalllalalaladpaalllllaapadpapaa'
    'aalaaaalllllapaappppaaaaaaldallllalllllllbdbaaaalalapaldllllppkaapdaallap'
    'alallllllappapkapppkaaaaallallllaaalldpaaalaaaadllllllllllkapplllllllllla'
    'llllallaaaaalalllllappapadpplaaalallllappapldlablladlllllllallkllapaaaddl'
    'alaaaaapadppaddlalaaaaaapappddlaaallapaaapaaaaalallalaaappaaaadlallllalap'
    'dpaaaddlaalaaaaapkalaladaalllllaaaalaaladaalllalaallalaadpaalllllllplppll'
    'llllaaaalllllakkpppapaladdlaaaaapppaplallllllllllkakaaaaalallllalaaaaaaal'
    'alllappaddlalaaaaaaadaallllllladdappadpppppaalalaaldllllllllllllkaaappaad'
    'llalllaaadalaalaaaalllblllllllddaaaaapaalllllllllppppadllalllllllllppalll'
    'allllllppppppaallallaaplaappdppaallaladllllllllllaaallllllallppppppppaala'
    'llaapppppladplllllladpallllldballllllddpplllllllllllllpkkllllllllllllllll'
    'lllllllllllllllbllaladapaalllllllalbllllllllllllbbllllllllllllllbblllllll'
    'lllbbbllllllllllbblaalaaaaabbbbbbbalalalallladdddbllalllllllllblllallllll'
    'bbbblllllallllllllalaaalaalaallaallaaalddddllallllbblllbbbbllalapppldapaa'
    'albldllallalllllllllllllllaapaallpppppdllaaalllllbaplbbbbbbllbbbbblllalll'
    'llllblbbldllllallllallllllllldllllallalllallllllldddddbbllllllllllldlllal'
    'lallallallllldlllalalalalalllallalllllallalalalaalllllallalaallalaaalllll'
    'lalalalalallllllalalllalalalalllllaaalllllalalalaallllllllllllaaalllallla'
    'aaaadaaaallaaaaallallllllalllalalallalllalalaladlllllllalllllaaaallalalaa'
    'llaalllaaaaaaaaaaaalllllllallllallalllllaallllaaaalllaalalaadallaaakkalll'
    'aaalllaaaaaaadlaallaaaallllllllaalallallaaalaadaaaaallllaaalall')

def main():
    for length in xrange(3, 8):
        data = collections.defaultdict(int)
        for i in xrange(len(DATA) - length):
            token = DATA[i:i+length]
            data[token] += 1
        result = []
        for k, v in data.iteritems():
            if v == 1:
                result.append(k)
        print '%d: %s' % (length, sorted(result))


if __name__ == '__main__':
    main()
